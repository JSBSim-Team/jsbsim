// Fill out your copyright notice in the Description page of Project Settings.


#include "JSBSimMovementComponent.h"
#include "JSBSimModule.h"

#pragma warning( push )
#pragma warning( disable : 4263 )
#pragma warning( disable : 4264 )
#pragma warning( disable : 4005 )

#include "FGFDMExec.h"
#include "math/FGLocation.h"
#include "models/FGFCS.h"
#include "models/FGPropulsion.h"
#include "models/propulsion/FGEngine.h"
#include "models/atmosphere/FGWinds.h"
#include "models/FGAerodynamics.h"
#include "models/FGAircraft.h"
#include "models/FGAtmosphere.h"
#include "models/FGAuxiliary.h"
#include "models/FGLGear.h"
#include "models/FGMassBalance.h"
#include "models/propulsion/FGThruster.h"
#include "models/propulsion/FGPiston.h"
#include "models/propulsion/FGTurbine.h"
#include "models/propulsion/FGTurboProp.h"
#include "models/propulsion/FGTank.h"
#include "initialization/FGInitialCondition.h"
#include "initialization/FGTrim.h"
#include "Interfaces/IPluginManager.h"

#pragma warning( pop )

#include "UEGroundCallback.h"

#include "DrawDebugHelpers.h"
#include "GeoReferencingSystem.h"
#include "Components/ActorComponent.h"



////// Constructor
UJSBSimMovementComponent::UJSBSimMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Add a stream redirector in order to output JSBSim debug info to Log
	// std::cout.rdbuf(&Stream);
}


////// Public functions
FString UJSBSimMovementComponent::GetAircraftScreenName() const
{
	FString ScreenName;
	if (AircraftLoaded && Aircraft)
	{
		ScreenName = FString(Aircraft->GetAircraftName().c_str());
	}

	return ScreenName;
}

void UJSBSimMovementComponent::LoadAircraft(bool ResetToDefaultSettings)
{
	UE_LOG(LogJSBSim, Display, TEXT("UJSBSimMovementComponent::LoadAircraft '0x%.8X' - %s"), this, *AircraftModel);

	// It seems like we can only load the model once after having been initialized - So we have to Reinit JSBSim when changing the model.
	DeInitializeJSBSim();
	InitializeJSBSim();
	AircraftLoaded = Exec->LoadModel(std::string(TCHAR_TO_UTF8(*AircraftModel)));

	if (!AircraftLoaded)
	{
		UE_LOG(LogJSBSim, Error, TEXT("Error while loading Model %s - Please check for typo or your configurations files"), *AircraftModel);
		return;
	}
	{
		UE_LOG(LogJSBSim, Display, TEXT("Model %s Loaded successfully !"), *AircraftModel);
	}

	// Do basic sanity checks 
	if (GroundReactions->GetNumGearUnits() <= 0)
	{
		UE_LOG(LogJSBSim, Error, TEXT("Error - Num Gear Units = %d. This is a very bad thing because with 0 gear units, the ground trimming routine will core dump"), GroundReactions->GetNumGearUnits());
		AircraftLoaded = false;
		return;
	}

	UpdateLocalTransforms();

	if (ResetToDefaultSettings)
	{
		// Do basic sanity checks 
		int EnginesCount = Propulsion->GetNumEngines();
		//UE_LOG(LogJSBSim, Display, TEXT("Number of engines : %d"), EnginesCount);



		// Initialize initial conditions
		// init_gear(); TODO - Why ?

		// TODO - Here - Consider the initial values of some variables in our actor. If they are set, they will override the config defaults. If not, we'll use the config default. 
		// 	For Each Tank
		//		- Fuel density-ppg 
		//		- Fuel level-lbs 
		//		- Capacity-gal_us - Readonly in BP-Editor

		InitTankDefaultProperties();
		InitGearDefaultProperties();
	}

	InitEnginesCommandAndStates();
}



////// ActorComponent overridables
void UJSBSimMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	double Start = FPlatformTime::Seconds();

	if (AircraftLoaded)
	{
		if (Crashed)
		{
			// TODO - Send event
			UE_LOG(LogJSBSim, Display, TEXT("Aircraft crashed..."));
		}
		else
		{
			// TODO - if paused ?? 

			// not needed - update ground cache. 
			Exec->Setdt(DeltaTime); // TODO Check with low framerate? integration step vs clock time step... 


			CopyToJSBSim();
			Exec->Run();
			UpdateLocalTransforms(); // TODO Est-ce obligé à chaque tick ? 
			CopyFromJSBSim();

			if (DrawDebug)
			{
				DrawDebugMessage();
				DrawDebugObjects();
			}

			if (Parent)
			{
				// Computes Rotation in engine frame
				FTransform ENUTransform = GeoReferencingSystem->GetTangentTransformAtECEFLocation(ECEFLocation);
				FRotator LocalUERotation(LocalEulerAngles);
				LocalUERotation.Yaw = LocalUERotation.Yaw - 90.0; // JSBSim heading is aero heading (0 at north). We have to remove 90 because in UE, 0 is pointing east. 
				FQuat EngineRotationQuat = ENUTransform.TransformRotation(LocalUERotation.Quaternion());
				
				FMatrix EngineRotation;
				EngineRotationQuat.ToMatrix(EngineRotation);
				FVector CGOffsetWorld = EngineRotation.TransformPosition(CGLocalPosition);

				

				// Computes Location in engine frame
				FVector CGWorldPosition;
				GeoReferencingSystem->ECEFToEngine(ECEFLocation, CGWorldPosition);

				FVector EngineLocation = CGWorldPosition - CGOffsetWorld;


				AircraftState.ForwardHorizontal = ENUTransform.TransformVector(ECEFForwardHorizontal);

				// Apply to actor			
				Parent->SetActorLocationAndRotation(EngineLocation, EngineRotationQuat);
			}
		}
	}

	double End = FPlatformTime::Seconds();

	TickTime = (End - Start) * 1000.0;
}

void UJSBSimMovementComponent::BeginDestroy()
{
	Super::BeginDestroy();
	DeInitializeJSBSim();
}

double UJSBSimMovementComponent::GetAGLevel(const FVector& StartECEFLocation, FVector& ECEFContactPoint, FVector& ECEFNormal)
{
	if (!GeoReferencingSystem || !GetWorld())
	{
		return 0.0;
	}


	FTransform TangentTransform = GeoReferencingSystem->GetTangentTransformAtECEFLocation(StartECEFLocation);
	FVector Up = TangentTransform.TransformVector(FVector::ZAxisVector);

	// Update HAT each step if not found previously, or scheduled by the threshold exceeded. 
	FVector StartEngineLocation;
	GeoReferencingSystem->ECEFToEngine(StartECEFLocation, StartEngineLocation);
	FVector LineCheckStart = StartEngineLocation + 200 * Up; // slightly above the starting point
	
	// AltitudeASL is in meters... 
	FVector LineCheckEnd = StartEngineLocation - (AircraftState.AltitudeASLFt * FEET_TO_METER + 0.05 * GeoReferencingSystem->GetGeographicEllipsoidMaxRadius()) * Up; // Estimate raycast length - Altitude + 5% of ellipsoid in case of negative altitudes
	FHitResult HitResult = FHitResult();

	static const FName LineTraceSingleName(TEXT("AGLevelLineTrace"));
	FCollisionQueryParams CollisionParams(LineTraceSingleName);
	CollisionParams.bTraceComplex = true;
	CollisionParams.AddIgnoredActor(Parent);

	FCollisionObjectQueryParams ObjectParams = FCollisionObjectQueryParams(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_Visibility);

	//DrawDebugLine(GetWorld(), LineCheckStart, LineCheckEnd , FColor::Blue, false, -1, 0, 3);

	double HAT = 0.0;
	if (GetWorld()->LineTraceSingleByObjectType(HitResult, LineCheckStart, LineCheckEnd, ObjectParams, CollisionParams))
	{
		//DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal*100.0, FColor::Orange, false, -1, 0, 3);


		FVector DirectionToImpact = HitResult.ImpactPoint - StartEngineLocation;
		HAT = FVector::Dist(StartEngineLocation, HitResult.ImpactPoint) / 100.0 * -FMath::Sign(DirectionToImpact.Dot(Up)); // JSBSim expect a signed distance. Consider that! 
		
		
		

		GeoReferencingSystem->EngineToECEF(HitResult.ImpactPoint, ECEFContactPoint);

		// Georeferencing don't provide tools to transform a direction, or access the worldToECEF Matrix - Do it by hand
		FVector ECEFNormalEnd;
		GeoReferencingSystem->EngineToECEF(HitResult.ImpactPoint + HitResult.ImpactNormal*100.0, ECEFNormalEnd);
		ECEFNormal = ECEFNormalEnd - ECEFContactPoint;
		ECEFNormal.Normalize();

	}
	else
	{
		ECEFContactPoint = FVector();
		ECEFNormal = FVector::ZAxisVector;
	}
	return HAT;
}

void UJSBSimMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	Parent = GetOwner();

	if (!GeoReferencingSystem)
	{
		GeoReferencingSystem = AGeoReferencingSystem::GetGeoReferencingSystem(GetWorld());

		if (!GeoReferencingSystem)
		{
			UE_LOG(LogJSBSim, Error, TEXT("Impossible to use a UJSBSimMovementComponent without a GeoReferencingSystem."));
		}
	}

	if (Parent)
	{
		Parent->GetRootComponent()->SetMobility(EComponentMobility::Movable);

		// TODO - Verifier si c'est bien necessaire sur un play. ça a ptet ete fait dans PostInitProperties
		LoadAircraft(false);
		PrepareJSBSim();
	}
	
}


void UJSBSimMovementComponent::OnRegister()
{
	Super::OnRegister();
	if (!GeoReferencingSystem)
	{
		GeoReferencingSystem = AGeoReferencingSystem::GetGeoReferencingSystem(GetWorld());

		if (!GeoReferencingSystem)
		{
			UE_LOG(LogJSBSim, Error, TEXT("Impossible to use a UJSBSimMovementComponent without a GeoReferencingSystem."));
		}
	}
}

/////////// JSBSim Protected methods

void UJSBSimMovementComponent::InitializeJSBSim()
{
	if (!JSBSimInitialized)
	{
		// ...
		Exec = new JSBSim::FGFDMExec();

		// Get pointers to main components
		Atmosphere = Exec->GetAtmosphere();
		Winds = Exec->GetWinds();
		FCS = Exec->GetFCS();
		MassBalance = Exec->GetMassBalance();
		Propulsion = Exec->GetPropulsion();
		Aircraft = Exec->GetAircraft();
		Propagate = Exec->GetPropagate();
		Auxiliary = Exec->GetAuxiliary();
		Inertial = Exec->GetInertial();
		Inertial->SetGroundCallback(new UEGroundCallback(this)); // Register ground callback.
		Aerodynamics = Exec->GetAerodynamics();
		GroundReactions = Exec->GetGroundReactions();
		Accelerations = Exec->GetAccelerations();
		IC = Exec->GetIC();

		// TODO - Global Settings somewhere ? 


		// Get the base directory of this plugin
		FString BaseDir = IPluginManager::Get().FindPlugin("JSBSimFlightDynamicsModel")->GetBaseDir();
		// Add on the relative location of the third party dll and load it
		FString RootDirRelative = FPaths::Combine(*BaseDir, TEXT("Resources/JSBSim"));
		const FString& RootDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*RootDirRelative);

		UE_LOG(LogJSBSim, Display, TEXT("Initializing JSBSimFlightDynamicsModel using Data in '%s' - '0x%.8X'"), *RootDir, this);
		// Set data paths...
		FString AircraftPath(TEXT("aircraft"));
		FString EnginePath(TEXT("engine"));
		FString SystemPath(TEXT("systems"));
		Exec->SetRootDir(SGPath(*RootDir));
		Exec->SetAircraftPath(SGPath(*AircraftPath));
		Exec->SetEnginePath(SGPath(*EnginePath));
		Exec->SetSystemsPath(SGPath(*SystemPath));

		// Prepare Initial Conditions
		TrimNeeded = true;

		//Exec->Setdt(0.016); // TODO -- Useful ? what is the meaning ? maybe just the substepping step. 
		// Base setup done so far. The other part of initial setup will be done on begin play, in InitJSBSim. 


		JSBSimInitialized = true;
	}
}

void UJSBSimMovementComponent::PrepareJSBSim()
{
	UE_LOG(LogJSBSim, Display, TEXT("PrepareJSBSim - Setting Initial Conditiond and computing initial state"));

	// We need the Aircraft to be loaded first
	if (!AircraftLoaded)
	{
		return;
	}

	// First, consider the aircraft Transform in UE to define the Location and Orientation in JSBSim
	FGeographicCoordinates GeographicCoordinates;
	if (Parent)
	{
		// Get CG location in Geographic
		FVector CGWorldPosition = Parent->GetTransform().TransformPosition(CGLocalPosition);
		GeoReferencingSystem->EngineToGeographic(CGWorldPosition, GeographicCoordinates);

		// Computes Rotation in engine frame
		FTransform ENUTransform = GeoReferencingSystem->GetTangentTransformAtGeographicLocation(GeographicCoordinates);
		FQuat LocalECEFRotation = ENUTransform.InverseTransformRotation(Parent->GetActorQuat());
		FRotator PsiThetaPhi = LocalECEFRotation.Rotator();

		// Set it as initial conditions
		IC->SetLongitudeDegIC(GeographicCoordinates.Longitude); 
		IC->SetGeodLatitudeDegIC(GeographicCoordinates.Latitude);
		IC->SetAltitudeASLFtIC(GeographicCoordinates.Altitude * METER_TO_FEET);
		//IC->SetAltitudeASLFtIC(GeographicCoordinates.Altitude * METER_TO_FEET);
		//IC->SetLatitudeDegIC(GeographicCoordinates.Latitude);
		//IC->SetLongitudeDegIC(GeographicCoordinates.Longitude);
		IC->SetPhiDegIC(PsiThetaPhi.Roll);
		IC->SetPsiDegIC(PsiThetaPhi.Yaw + 90);
		IC->SetThetaDegIC(PsiThetaPhi.Pitch);
	}


	// Init our commands from the initial state
	Commands.Elevator = FCS->GetDeCmd();
	Commands.Aileron = FCS->GetDaCmd();
	Commands.Rudder = FCS->GetDrCmd();
	Commands.PitchTrim = FCS->GetPitchTrimCmd();
	Commands.YawTrim = FCS->GetYawTrimCmd();
	Commands.RollTrim = FCS->GetRollTrimCmd();
	// TODO - Others commands ?
	// ThrottleCommand = FCS->GetThrottleCmd(0); // TODO - For each Engine

	// Reset the current Aircraft State
	AircraftState.Reset();


	// Atmosphere
	if (ControlFDMAtmosphere)
	{
		Atmosphere->SetTemperature(TemperatureCelsius, GeographicCoordinates.Altitude * METER_TO_FEET, JSBSim::FGAtmosphere::eCelsius);
		Atmosphere->SetPressureSL(JSBSim::FGAtmosphere::ePascals, PressureSeaLevelhPa * 100.0);
		Winds->SetTurbType(JSBSim::FGWinds::ttNone);
		Winds->SetTurbGain(0.0);
		Winds->SetTurbRate(0.0);
		Winds->SetWindspeed20ft(0.0);
		Winds->SetProbabilityOfExceedence(0.0);
	}
	
	// Wind Speed
	IC->SetWindDirDegIC(WindHeading);
	IC->SetWindMagKtsIC(WindIntensityKts);
	
	// Aircraft Speed
	IC->SetVcalibratedKtsIC(InitialCalibratedAirSpeedKts);


	// Aircraft State
	// Flaps position
	FCS->SetDfPos(ofNorm, FlapPositionAtStart);
	// Gear position
	if (bStartWithGearDown)
	{
		FCS->SetGearPos(1.0);
	}
	else
	{
		FCS->SetGearPos(0.0);
	}

	// Tanks / Fuel
	


	// Engines - Starter / Running
	// Foreach Engine 
	//	- Set RPM (rpm/gearRatio) 

	// To set in UE because it's not in the config files
	// 	- FuelFreeze
	//	- Stall Warning
	//  - BOOL ControlFDMAtmosphere
	//		- Temperature
	//		- Pressure
	//		- PressureSL
	//		- altitude ? 
	//		- ground_wind
	//		- turbulence_gain
	//		- turbulence_rate
	//		- turbulence_model
	//		- wind_from_north
	//		- wind_from_east
	//		- wind_from_down
	

	CopyToJSBSim();
	Exec->RunIC();
	UpdateLocalTransforms();

	if (bStartWithEngineRunning)
	{
		
		Propulsion->InitRunning(-1);

		// TODO - Take the state of the engine as Commands... 
		int32 EngineCount = EngineCommands.Num();
		for (int32 i = 0; i < EngineCount; i++)
		{
			EngineCommands[i].Throttle = 0.0;
			EngineCommands[i].Mixture = 0.0;
			EngineCommands[i].Running = true;
		}
		// Get Engines States
		/*for (unsigned int i = 0; i < Propulsion->GetNumEngines(); i++) {
			FGPiston* eng = (FGPiston*)Propulsion->GetEngine(i);
			globals->get_controls()->set_magnetos(i, eng->GetMagnetos());
			globals->get_controls()->set_mixture(i, FCS->GetMixtureCmd(i));
		}*/
	}


	if (TrimNeeded)
	{
		const JSBSim::FGLocation& cart = IC->GetPosition();
		//double cart_pos[3], contact[3], d[3], vel[3], agl;
		// Make sure the ground below is loaded... Not needed in our case...
		//update_ground_cache(cart, cart_pos, 0.01);

		// Get Above Ground Level -- TODO
		//get_agl_ft(fdmex->GetSimTime(), cart_pos, SG_METER_TO_FEET * 2, contact, d, vel, d, &agl);
		double terrain_elevation = 0;
		//double terrain_alt = sqrt(contact[0] * contact[0] + contact[1] * contact[1] + contact[2] * contact[2]) - cart.GetSeaLevelRadius();

		//UE_LOG(LogJSBSim, Display, TEXT("Ready to trim, terrain elevation is: %f"), terrain_elevation);
		double vel[3] = {0,0,0};

		if (StartOnGround) {
			FGColumnVector3 gndVelNED = cart.GetTec2l() * FGColumnVector3(vel[0], vel[1], vel[2]);
			IC->SetVNorthFpsIC(gndVelNED(1));
			IC->SetVEastFpsIC(gndVelNED(2));
			IC->SetVDownFpsIC(gndVelNED(3));
		}
		DoTrim();
		TrimNeeded = false;
	}


	CopyFromJSBSim();

	

	//LogInitialization();

}

void UJSBSimMovementComponent::DeInitializeJSBSim()
{
	if (JSBSimInitialized)
	{
		UE_LOG(LogJSBSim, Display, TEXT("DeInitializeJSBSim - '0x%.8X'"), this);

		if (Exec)
		{
			delete Exec;
			Exec = nullptr;

			Atmosphere = nullptr;
			Winds = nullptr;
			FCS = nullptr;
			MassBalance = nullptr;
			Propulsion = nullptr;
			Aircraft = nullptr;
			Propagate = nullptr;
			Auxiliary = nullptr;
			Inertial = nullptr;
			Aerodynamics = nullptr;
			GroundReactions = nullptr;
			Accelerations = nullptr;
			IC = nullptr;
		}

		JSBSimInitialized = false;
	}
}

void UJSBSimMovementComponent::CopyToJSBSim()
{
	// Basic flight controls
	FCS->SetDaCmd(Commands.Aileron);
	FCS->SetRollTrimCmd(Commands.RollTrim);
	FCS->SetDeCmd(Commands.Elevator);
	FCS->SetPitchTrimCmd(Commands.PitchTrim);
	FCS->SetDrCmd(-Commands.Rudder); // Rudder
	FCS->SetDsCmd(Commands.Rudder); // Steering
	FCS->SetYawTrimCmd(-Commands.YawTrim);
	FCS->SetDfCmd(Commands.Flap);
	FCS->SetDsbCmd(Commands.SpeedBrake);
	FCS->SetDspCmd(Commands.Spoiler);

	  
	// Gears and Brake controls

	double parking_brake = Commands.ParkingBrake;
	double left_brake = Commands.LeftBrake;
	double right_brake = Commands.RightBrake;
	/*if (ab_brake_engaged->getBoolValue()) {
		left_brake = ab_brake_left_pct->getDoubleValue();
		right_brake = ab_brake_right_pct->getDoubleValue();
	}*/
	FCS->SetLBrake(FMath::Max(left_brake, parking_brake));
	FCS->SetRBrake(FMath::Max(right_brake, parking_brake));
	FCS->SetCBrake(0.0);

	FCS->SetGearCmd(Commands.GearDown); 


	ApplyEnginesCommands(); 

	


	// TODO - Update atmosphere
	
	//Atmosphere->SetTemperature(temperature->getDoubleValue(), get_Altitude(), FGAtmosphere::eCelsius);
	//Atmosphere->SetPressureSL(FGAtmosphere::eInchesHg, pressureSL->getDoubleValue());

	//Winds->SetTurbType((FGWinds::tType)TURBULENCE_TYPE_NAMES[turbulence_model->getStringValue()]);
	//switch (Winds->GetTurbType()) {
	//case FGWinds::ttStandard:
	//case FGWinds::ttCulp: {
	//	double tmp = turbulence_gain->getDoubleValue();
	//	Winds->SetTurbGain(tmp * tmp * 100.0);
	//	Winds->SetTurbRate(turbulence_rate->getDoubleValue());
	//	break;
	//}
	//case FGWinds::ttMilspec:
	//case FGWinds::ttTustin: {
	//	// milspec turbulence: 3=light, 4=moderate, 6=severe turbulence
	//	// turbulence_gain normalized: 0: none, 1/3: light, 2/3: moderate, 3/3: severe
	//	double tmp = turbulence_gain->getDoubleValue();
	//	Winds->SetProbabilityOfExceedence(
	//		SGMiscd::roundToInt(TurbulenceSeverityTable.GetValue(tmp))
	//	);
	//	Winds->SetWindspeed20ft(ground_wind->getDoubleValue());
	//	break;
	//}

	//default:
	//	break;
	//}

	//Winds->SetWindNED(-wind_from_north->getDoubleValue(),
	//	-wind_from_east->getDoubleValue(),
	//	-wind_from_down->getDoubleValue());
	////    SG_LOG(SG_FLIGHT,SG_INFO, "Wind NED: "
	////                  << get_V_north_airmass() << ", "
	////                  << get_V_east_airmass()  << ", "
	////                  << get_V_down_airmass() );

	CopyTankPropertiesToJSBSim();
	CopyGearPropertiesToJSBSim();
}
void UJSBSimMovementComponent::CopyFromJSBSim()
{
	// Collect JSBSim data
	Propagate->DumpState();
	
	VelocityNEDfps.Set(Propagate->GetVel(JSBSim::FGJSBBase::eNorth), Propagate->GetVel(JSBSim::FGJSBBase::eEast), -Propagate->GetVel(JSBSim::FGJSBBase::eDown));
	EulerRates.Set(Auxiliary->GetEulerRates(JSBSim::FGJSBBase::ePhi), Auxiliary->GetEulerRates(JSBSim::FGJSBBase::eTht), Auxiliary->GetEulerRates(JSBSim::FGJSBBase::ePsi));


	// Location in ECEF
	FVector FormerECEFLocation = ECEFLocation;

	JSBSim::FGLocation LocationVRP = Propagate->GetLocation();
	ECEFLocation = FVector(LocationVRP(1), LocationVRP(2), LocationVRP(3)) * FEET_TO_METER;
	
	
	// Get Aircraft forward vector in local (ECEF tangent) space. 
	// TODO - IDK if for the horizon indicator I should use the forward vector or the aircraft speed. 
	// Maybe the aircraft speed would include some kind of lateral slip ---> May one expert fix it if needed...
	JSBSim::FGColumnVector3 ForwardLocal = Propagate->GetTb2l() * JSBSim::FGColumnVector3(1,0,0);
	ECEFForwardHorizontal = FVector(ForwardLocal(2), -ForwardLocal(1), 0);
	
	// Compute Instant speed in FPS
	// AVIRER - C'est juste pour voir si notre vitesse UE est correcte vs celle reportee par JSBSim
	if (GetWorld())
	{
		FVector DeltaLocation = ECEFLocation - FormerECEFLocation;
		InstantSpeedFPS = DeltaLocation.Size() / GetWorld()->GetDeltaSeconds() * METER_TO_FEET;
	}
	else
	{
		InstantSpeedFPS = 0;
	}
	

	// TODO - Passer en World Frame / Round Planet... 
	LocalEulerAngles.Yaw = FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::ePsi)); 
	LocalEulerAngles.Pitch = FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::eTht));
	LocalEulerAngles.Roll = FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::ePhi));
	
	//UE_LOG(LogJSBSim, Display, TEXT("State : Lat %f - Long %f - Alt %f    Bank %f  Pitch %f  Heading %f"), Latitude, Longitude, Altitude, Bank, Pitch, Heading);


	AircraftState.GroundSpeedKts = Auxiliary->GetVground() * FEET_PER_SEC_TO_KNOT;
	AircraftState.CalibratedAirSpeedKts = Auxiliary->GetVcalibratedKTS();
	AircraftState.AltitudeASLFt = Propagate->GetAltitudeASL();
	AircraftState.HeadingDeg = LocalEulerAngles.Yaw;
	AircraftState.StallWarning = Aerodynamics->GetStallWarn();
	AircraftState.AltitudeRateFtps = Propagate->Gethdot();


	TotalVelocityfps = Auxiliary->GetVt();
	

	// Update Moving part state
	AircraftState.ElevatorPosition = FCS->GetDePos(JSBSim::ofDeg);
	AircraftState.LeftAileronPosition = FCS->GetDaLPos(JSBSim::ofDeg);
	AircraftState.RightAileronPosition = FCS->GetDaRPos(JSBSim::ofDeg);
	AircraftState.RudderPosition = -1 * FCS->GetDrPos(JSBSim::ofDeg);
	AircraftState.FlapPosition = FCS->GetDfPos(JSBSim::ofDeg);
	AircraftState.SpeedBrakePosition = FCS->GetDsbPos(JSBSim::ofDeg);
	AircraftState.SpoilersPosition = FCS->GetDspPos(JSBSim::ofDeg);
	//tailhook_pos_pct->setDoubleValue(FCS->GetTailhookPos());
	//wing_fold_pos_pct->setDoubleValue(FCS->GetWingFoldPos());


	// Copy the fuel levels from JSBSim if fuel
	// freeze not enabled.

	CopyTankPropertiesFromJSBSim();
	CopyGearPropertiesFromJSBSim();
	GetEnginesStates();


	// force a sim crashed if crashed (altitude AGL < 0)
	if (Propagate->GetDistanceAGL() < -10.0) {
		Exec->SuspendIntegration();
		Crashed = true;
	}

}

/////////// JSBSim Private methods
void UJSBSimMovementComponent::DoTrim()
{
	JSBSim::FGTrim* Trim;

	if (StartOnGround)
	{
		Trim = new JSBSim::FGTrim(Exec, JSBSim::tGround);
	}
	else {
		Trim = new JSBSim::FGTrim(Exec, JSBSim::tFull);
	}

	if (!Trim->DoTrim()) 
	{
		Trim->Report();
		Trim->TrimStats();
	}
	else 
	{
		Trimmed = true;
	}

	delete Trim;


	Commands.PitchTrim = FCS->GetPitchTrimCmd();
	Commands.Aileron = FCS->GetDaCmd();
	Commands.Rudder = -FCS->GetDrCmd(); // TODO Pourquoi "-" ??


	// Why globals ? 
	/*globals->get_controls()->set_elevator_trim(FCS->GetPitchTrimCmd());
	globals->get_controls()->set_elevator(FCS->GetDeCmd());
	for (unsigned i = 0; i < Propulsion->GetNumEngines(); i++)
		globals->get_controls()->set_throttle(i, FCS->GetThrottleCmd(i));

	globals->get_controls()->set_aileron(FCS->GetDaCmd());
	globals->get_controls()->set_rudder(-FCS->GetDrCmd());*/

	//UE_LOG(LogJSBSim, Display, TEXT("Trim Complete - PitchTrim = %f, Throttle = %f, Aileron = %f, Rudder = %f"), PitchTrimCommand, ThrottleCommand, AileronCommand, RudderCommand);

}

void UJSBSimMovementComponent::UpdateLocalTransforms()
{
	// Structural Frame To Actor Frame
	FMatrix StructuralToActorMatrix(FMatrix::Identity);
	StructuralToActorMatrix.SetAxis(0, FVector(-1, 0, 0));
	StructuralToActorMatrix.SetAxis(1, FVector(0, 1, 0));
	StructuralToActorMatrix.SetAxis(2, FVector(0, 0, 1));
	StructuralToActorMatrix.SetOrigin(StructuralFrameOrigin);
	StructuralToActor.SetFromMatrix(StructuralToActorMatrix);


	// Get Gravity Center
	JSBSim::FGColumnVector3 CGLocationStructural = MassBalance->StructuralToBody(JSBSim::FGColumnVector3()) * FEET_TO_CENTIMETER;
	CGLocalPosition = StructuralToActor.TransformPosition(FVector(CGLocationStructural(1), CGLocationStructural(2), CGLocationStructural(3)));

	// Body Frame to Actor Frame
	FMatrix BodyToActorMatrix(FMatrix::Identity);
	BodyToActorMatrix.SetAxis(0, FVector(1, 0, 0));
	BodyToActorMatrix.SetAxis(1, FVector(0, 1, 0));
	BodyToActorMatrix.SetAxis(2, FVector(0, 0, -1));
	BodyToActorMatrix.SetOrigin(CGLocalPosition);
	BodyToActor.SetFromMatrix(BodyToActorMatrix);


	// Eye Position
	JSBSim::FGColumnVector3 EPLocationStructural = Aircraft->GetXYZep() * INCH_TO_CENTIMETER;
	EPLocalPosition = StructuralToActor.TransformPosition(FVector(EPLocationStructural(1), EPLocationStructural(2), EPLocationStructural(3)));

	// Visual Reference Position
	JSBSim::FGColumnVector3 VRPLocationStructural = Aircraft->GetXYZvrp() * INCH_TO_CENTIMETER;
	VRPLocalPosition = StructuralToActor.TransformPosition(FVector(VRPLocationStructural(1), VRPLocationStructural(2), VRPLocationStructural(3)));



}

// Gears
void UJSBSimMovementComponent::InitGearDefaultProperties()
{
	uint32 GearsCount = GroundReactions->GetNumGearUnits();
	//UE_LOG(LogJSBSim, Display, TEXT("Number of Gears : %d"), GearsCount);
	Gears.Empty();
	if (GearsCount > 0)
	{
		Gears.AddZeroed(GearsCount);

		for (unsigned int i = 0; i < GearsCount; i++)
		{
			std::shared_ptr<JSBSim::FGLGear> Gear = GroundReactions->GetGearUnit(i); // TODO - Test indices

			Gears[i].NormalizedPosition = Gear->GetGearUnitPos();
			Gears[i].IsBogey = Gear->IsBogey();
			Gears[i].HasWeightOnWheel = Gear->GetWOW();
			Gears[i].WheelRollLinearVelocity_mps = Gear->GetWheelRollVel() * FEET_TO_METER;
			Gears[i].IsUp = Gear->GetGearUnitUp();
			Gears[i].IsDown = Gear->GetGearUnitDown();
			Gears[i].Name = FString(Gear->GetName().c_str());
		}
	}
}

void UJSBSimMovementComponent::CopyGearPropertiesToJSBSim()
{
	// TODO - Update Gears ? Ex : Etat initial Up/Down
	// Not sure it can be done...

	//uint32 GearsCount = GroundReactions->GetNumGearUnits();
	//if (GearsCount > 0)
	//{
	//	for (unsigned int i = 0; i < GearsCount; i++)
	//	{
	//		JSBSim::FGLGear* Gear = GroundReactions->GetGearUnit(i); // TODO - Test indices

	//		Gear->SetPosition() = Gears[i].NormalizedPosition;
	//	}
	//}
}
	

void UJSBSimMovementComponent::CopyGearPropertiesFromJSBSim()
{
	for (int i = 0; i < GroundReactions->GetNumGearUnits(); i++)
	{
		std::shared_ptr<JSBSim::FGLGear> Gear = GroundReactions->GetGearUnit(i); // TODO - Test indices
		if ((int32)i < Gears.Num())
		{
			Gears[i].NormalizedPosition = Gear->GetGearUnitPos();
			Gears[i].IsBogey = Gear->IsBogey();
			Gears[i].HasWeightOnWheel = Gear->GetWOW();
			Gears[i].WheelRollLinearVelocity_mps = Gear->GetWheelRollVel() * FEET_TO_METER;
			Gears[i].IsUp = Gear->GetGearUnitUp();
			Gears[i].IsDown = Gear->GetGearUnitDown();

			JSBSim::FGColumnVector3 GearBodyLocation = Gear->GetBodyLocation() * FEET_TO_CENTIMETER;
			Gears[i].RelativeLocation = BodyToActor.TransformPosition(FVector(GearBodyLocation(1), GearBodyLocation(2), GearBodyLocation(3)));

			JSBSim::FGColumnVector3 GearBodyForce = Gear->GetBodyForces() * FEET_TO_CENTIMETER;
			Gears[i].Force = BodyToActor.TransformPosition(FVector(GearBodyForce(1), GearBodyForce(2), GearBodyForce(3)));
		}
	}
}

// Tanks
void UJSBSimMovementComponent::InitTankDefaultProperties()
{
	// Set initial fuel levels if provided.
	uint32 TanksCount = Propulsion->GetNumTanks();
	//UE_LOG(LogJSBSim, Display, TEXT("Number of Tanks : %d"), TanksCount);
	Tanks.Empty();
	if (TanksCount > 0)
	{
		Tanks.AddZeroed(TanksCount);
		//Tanks.SetNum(TanksCount);
		for (unsigned int i = 0; i < TanksCount; i++)
		{
			std::shared_ptr<JSBSim::FGTank> Tank = Propulsion->GetTank(i); // TODO - Test indices

			Tanks[i].FuelDensityPoundsPerGallon = Tank->GetDensity();
			Tanks[i].ContentGallons = Tank->GetContentsGallons();
			Tanks[i].CapacityGallons = Tank->GetCapacityGallons();
			Tanks[i].FillPercentage = Tank->GetPctFull();
			Tanks[i].TemperatureCelcius = Tank->GetTemperature_degC();
		}
	}
}

void UJSBSimMovementComponent::CopyTankPropertiesToJSBSim()
{
	// TODO - Update Tanks
	for (unsigned i = 0; i < Propulsion->GetNumTanks(); i++)
	{
		std::shared_ptr<JSBSim::FGTank> tank = Propulsion->GetTank(i);

		if ((int32)i < Tanks.Num())
		{
			FTank UETank = Tanks[i];

			double fuelDensity = UETank.FuelDensityPoundsPerGallon;

			if (fuelDensity < 0.1)
				fuelDensity = 6.0; // Use average fuel value

			// Only editable properties
			tank->SetDensity(fuelDensity);
			tank->SetContentsGallons(UETank.ContentGallons);
		}

	}
}

void UJSBSimMovementComponent::CopyTankPropertiesFromJSBSim()
{
	FuelFreeze = Propulsion->GetFuelFreeze();
	for (unsigned int i = 0; i < Propulsion->GetNumTanks(); i++)
	{
		std::shared_ptr<JSBSim::FGTank> Tank = Propulsion->GetTank(i);

		if ((int32)i < Tanks.Num())
		{
			Tanks[i].FuelDensityPoundsPerGallon = Tank->GetDensity();
			Tanks[i].ContentGallons = Tank->GetContentsGallons();
			Tanks[i].CapacityGallons = Tank->GetCapacityGallons();
			Tanks[i].FillPercentage = Tank->GetPctFull();
			Tanks[i].TemperatureCelcius = Tank->GetTemperature_degC();


			//double contents = tank->GetContents();
			//double temp = tank->GetTemperature_degC();
			//double fuelDensity = tank->GetDensity();

			//if (fuelDensity < 0.1)
			//	fuelDensity = 6.0; // Use average fuel value

			//node->setDoubleValue("density-ppg", fuelDensity);
			//node->setDoubleValue("level-lbs", contents);
			//if (temp != -9999.0) node->setDoubleValue("temperature_degC", temp);

			//node->setDoubleValue("arm-in", tank->GetXYZ(FGJSBBase::eX));
		}
	}

}


// Engines

void UJSBSimMovementComponent::InitEnginesCommandAndStates()
{
	EngineCommands.Empty();
	EngineStates.Empty();

	uint32 EngineCount = Propulsion->GetNumEngines();
	if (EngineCount > 0)
	{
		// Allocate UE equivalent structures
		EngineCommands.AddZeroed(EngineCount);
		EngineStates.AddZeroed(EngineCount);

		// Apply default properties
		// TODO - Not sure there are to apply. it will be done by command/states
		//for (unsigned int i = 0; i < EngineCount; i++)
		//{
		//}
	}
}

void UJSBSimMovementComponent::ApplyEnginesCommands()
{
	// Global to all engines
	Propulsion->SetFuelFreeze(FuelFreeze);

	// For each engine
	int32 EngineCount = EngineCommands.Num();
	for (int32 i = 0; i < EngineCount; i++)
	{
		FEngineCommand EngineCommand = EngineCommands[i];

		// Global FCS Commands
		FCS->SetThrottleCmd(i, EngineCommand.Throttle);
		FCS->SetMixtureCmd(i, EngineCommand.Mixture);
		FCS->SetPropAdvanceCmd(i, EngineCommand.PropellerAdvance);
		FCS->SetFeatherCmd(i, EngineCommand.PropellerFeather);

		// Common FGEngine code block
		std::shared_ptr < JSBSim::FGEngine> CommonEngine = Propulsion->GetEngine(i);
		CommonEngine->SetStarter(EngineCommand.Starter);
		CommonEngine->SetRunning(EngineCommand.Running);

		switch (Propulsion->GetEngine(i)->GetType())
		{
		case JSBSim::FGEngine::etPiston:
		{
			// FGPiston code block
			std::shared_ptr < JSBSim::FGPiston> PistonEngine = std::static_pointer_cast<JSBSim::FGPiston>(Propulsion->GetEngine(i));
			PistonEngine->SetMagnetos(EngineCommand.Magnetos);
			break;
		}
		case JSBSim::FGEngine::etTurbine:
		{
			// FGTurbine code block
			std::shared_ptr < JSBSim::FGTurbine> TurbineEngine = std::static_pointer_cast<JSBSim::FGTurbine>(Propulsion->GetEngine(i));
			TurbineEngine->SetReverse(EngineCommand.Reverse);
			TurbineEngine->SetCutoff(EngineCommand.CutOff);
			TurbineEngine->SetIgnition(EngineCommand.Ignition);
			TurbineEngine->SetAugmentation(EngineCommand.Augmentation);
			TurbineEngine->SetInjection(EngineCommand.Injection);
			break;
		}
		case JSBSim::FGEngine::etRocket:
		{
			// FGRocket code block
			// FGRocket* RocketEngine = (FGRocket*)Propulsion->GetEngine(i);
			break;
		}
		case JSBSim::FGEngine::etTurboprop:
		{
			// FGTurboProp code block
			std::shared_ptr < JSBSim::FGTurboProp> TurboPropEngine = std::static_pointer_cast<JSBSim::FGTurboProp>(Propulsion->GetEngine(i));
			TurboPropEngine->SetReverse(EngineCommand.Reverse);
			TurboPropEngine->SetCutoff(EngineCommand.CutOff);
			TurboPropEngine->SetGeneratorPower(EngineCommand.GeneratorPower);
			TurboPropEngine->SetCondition(EngineCommand.Condition);
			break;
		}
		default:
			break;
		}
	}
}


void UJSBSimMovementComponent::GetEnginesStates()
{
	// For each engine
	int32 EngineCount = EngineStates.Num();
	for (int32 i = 0; i < EngineCount; i++)
	{
		std::shared_ptr < JSBSim::FGEngine> Engine = Propulsion->GetEngine(i);

		EngineStates[i].EngineType = (EEngineType) Engine->GetType();
		EngineStates[i].Starter = Engine->GetStarter();
		EngineStates[i].Running = Engine->GetRunning();
		EngineStates[i].Thrust = Engine->GetThrust();
		EngineStates[i].EngineRPM = Engine->GetThruster()->GetEngineRPM();

		switch (Engine->GetType())
		{
		case JSBSim::FGEngine::etPiston:
		{
			// TODO
			//// FGPiston code block
			//JSBSim::FGPiston* PistonEngine = (JSBSim::FGPiston*)Propulsion->GetEngine(i);
			//PistonEngine->SetMagnetos(EngineCommand.Magnetos);
			break;
		}
		case JSBSim::FGEngine::etTurbine:
		{
			// TODO
			// FGTurbine code block
			std::shared_ptr < JSBSim::FGTurbine> TurbineEngine = std::static_pointer_cast<JSBSim::FGTurbine>(Engine);
			EngineStates[i].N1 = TurbineEngine->GetN1();
			EngineStates[i].N2 = TurbineEngine->GetN2();
			EngineStates[i].Augmentation = TurbineEngine->GetAugmentation();
			EngineStates[i].Reversed = TurbineEngine->GetReversed();
			EngineStates[i].Injection = TurbineEngine->GetInjection();
			EngineStates[i].CutOff = TurbineEngine->GetCutoff();
			EngineStates[i].Ignition = TurbineEngine->GetIgnition();
			break;
		}
		case JSBSim::FGEngine::etRocket:
		{
			// TODO
			// FGRocket code block
			// FGRocket* RocketEngine = (FGRocket*)Propulsion->GetEngine(i);
			break;
		}
		case JSBSim::FGEngine::etTurboprop:
		{
			// TODO
			//// FGTurboProp code block
			//JSBSim::FGTurboProp* TurboPropEngine = (JSBSim::FGTurboProp*)Propulsion->GetEngine(i);
			//TurboPropEngine->SetReverse(EngineCommand.Reverse);
			//TurboPropEngine->SetCutoff(EngineCommand.CutOff);
			//TurboPropEngine->SetGeneratorPower(EngineCommand.GeneratorPower);
			//TurboPropEngine->SetCondition(EngineCommand.Condition);
			break;
		}
		default:
			break;
		}
	}
}

/////////// Logging and Debugging Methods

void UJSBSimMovementComponent::LogInitialization()
{
	UE_LOG(LogJSBSim, Display, TEXT("Initialized JSB Sim with : "));


	// Speed
	switch (IC->GetSpeedSet())
	{
		//typedef enum { setvt, setve, setvg } speedset; ??? 
	case JSBSim::setned: // North East Down
		UE_LOG(LogJSBSim, Display, TEXT("  Vn,Ve,Vd= %f, $f, $f  ft/s"), Propagate->GetVel(JSBSim::FGJSBBase::eNorth), Propagate->GetVel(JSBSim::FGJSBBase::eEast), Propagate->GetVel(JSBSim::FGJSBBase::eDown));
		break;
	case JSBSim::setuvw:
		UE_LOG(LogJSBSim, Display, TEXT("  U,V,W= %f, $f, $f  ft/s"), Propagate->GetUVW(1), Propagate->GetUVW(2), Propagate->GetUVW(3));
		break;
	case JSBSim::setmach:
		UE_LOG(LogJSBSim, Display, TEXT("  Mach: %f, $f, $f  ft/s"), Auxiliary->GetMach());
		break;
	case JSBSim::setvc:
	default:
		UE_LOG(LogJSBSim, Display, TEXT("  Indicated Airspeed: %f knots"), Auxiliary->GetVcalibratedKTS());
		break;
	}
	
	// Angles
	UE_LOG(LogJSBSim, Display, TEXT("  Bank: %f, Pitch: %f, True Heading: %f"), 
		FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::ePhi)),
		FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::eTht)),
		FMath::RadiansToDegrees(Propagate->GetEuler(JSBSim::FGJSBBase::ePsi)));

	// Lat/Long
	UE_LOG(LogJSBSim, Display, TEXT("  Latitude: %f, Longitude: %f deg, Altitude: %f feet"), Propagate->GetLocation().GetLatitudeDeg(),	Propagate->GetLocation().GetLongitudeDeg(),	Propagate->GetAltitudeASL());

}
void UJSBSimMovementComponent::DrawDebugMessage()
{
	FVector2D TextScale = FVector2D::UnitVector;

	// Build message
	FString DebugMessage;

	// Inputs
	DebugMessage += Commands.GetDebugMessage();

	DebugMessage += LINE_TERMINATOR;
	DebugMessage += TEXT("Outputs :"); DebugMessage += LINE_TERMINATOR;

	// Engines // TODO - Iterate over UE objects and not JSBSim ones to make sur it's Ok in UE side
	DebugMessage += LINE_TERMINATOR;
	int32 NumEngines = EngineCommands.Num();
	DebugMessage += FString::Printf(TEXT("Engines (%d) : "), NumEngines) + LINE_TERMINATOR;
	for (int32 i = 0; i < NumEngines; i++)
	{
		DebugMessage += FString::Printf(TEXT("  #%d - Command :"), i) + LINE_TERMINATOR;
		DebugMessage += EngineCommands[i].GetDebugMessage();
		DebugMessage += FString::Printf(TEXT("  #%d - State :"), i) + LINE_TERMINATOR;
		DebugMessage += EngineStates[i].GetDebugMessage();
	}

	// Tanks // TODO - Iterate over UE objects and not JSBSim ones to make sur it's Ok in UE side
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("Tanks (%d) : "), Propulsion->GetNumTanks()) + LINE_TERMINATOR;
	uint8 TankIndex = 0;
	for (FTank Tank : Tanks)
	{
		TankIndex++;
		DebugMessage += FString::Printf(TEXT("  #%d - Content %.2f / %.2f gal [%.1f %%], Temp %.1f C, Density %.2f ppg"), TankIndex, Tank.ContentGallons, Tank.CapacityGallons, Tank.FillPercentage, Tank.TemperatureCelcius, Tank.FuelDensityPoundsPerGallon) + LINE_TERMINATOR;
	}

	// GEAR
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("Landing Gears (%d) : "), GroundReactions->GetNumGearUnits()) + LINE_TERMINATOR;
	
	/*for (int i = 0; i < GroundReactions->GetNumGearUnits(); i++)
	{
		JSBSim::FGLGear* Gear = GroundReactions->GetGearUnit(i);
		if (Gear->IsBogey())
		{
			DebugMessage += FString::Printf(TEXT("  #%d - Up %d Down %d WOW %d / Position %.2f Compression %.2f"), i, Gear->GetGearUnitUp(), Gear->GetGearUnitDown(), Gear->GetWOW(), Gear->GetGearUnitPos(), Gear->GetCompLen()) + LINE_TERMINATOR;

		}
	}*/
	uint8 GearIndex = 0;
	for (FGear Gear : Gears)
	{
		GearIndex++;
		if (Gear.IsBogey)
		{
			DebugMessage += FString::Printf(TEXT("  #%d - Up %d Down %d WOW %d / Position %.2f WheelVel %.2f Force %.2f"), GearIndex, Gear.IsUp, Gear.IsDown, Gear.HasWeightOnWheel, Gear.NormalizedPosition, Gear.WheelRollLinearVelocity_mps, Gear.Force.Size()) + LINE_TERMINATOR;
		}
	}
	

	// State 
	DebugMessage += LINE_TERMINATOR;
	DebugMessage += TEXT("State :"); DebugMessage += LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("  X %f  Y %f  Z %f"), ECEFLocation.X, ECEFLocation.Y, ECEFLocation.Z) + LINE_TERMINATOR;

	DebugMessage += FString::Printf(TEXT("  Lat %f  Long %f "), Propagate->GetLatitudeDeg(), Propagate->GetLongitudeDeg()) + LINE_TERMINATOR;

	DebugMessage += FString::Printf(TEXT("  Heading %f Pitch %f Roll %f"), LocalEulerAngles.Yaw, LocalEulerAngles.Pitch, LocalEulerAngles.Roll) + LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("  GroundSpeed %f fps, InstantSpeed %f mps, JSBSimDt %f WorldDt %f SimTime %.2f WorldTime %.2f TickTime %.3f ms"), AircraftState.GroundSpeedKts, InstantSpeedFPS, Exec->GetDeltaT(), GetWorld()->GetDeltaSeconds(), Exec->GetSimTime(), GetWorld()->TimeSeconds, TickTime) + LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("  Speed NED %s"), *VelocityNEDfps.ToString()) + LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("  Euler Rate %s"), *EulerRates.ToString()) + LINE_TERMINATOR;
	DebugMessage += FString::Printf(TEXT("  AltitudeASL %f Ft"), AircraftState.AltitudeASLFt) + LINE_TERMINATOR;

	JSBSim::FGColumnVector3 BodyLocation = MassBalance->StructuralToBody(JSBSim::FGColumnVector3()) * FEET_TO_METER * 100.0;
	

	DebugMessage += FString::Printf(TEXT("  Structural to Body %.4f %.4f %.4f"), BodyLocation(1), BodyLocation(2), BodyLocation(3)) + LINE_TERMINATOR;


	DebugMessage += LINE_TERMINATOR;
	DebugMessage += AircraftState.GetDebugMessage();

	// Draw
	GEngine->AddOnScreenDebugMessage(1, 0, FColor::Green, *DebugMessage, false, TextScale);
}

void UJSBSimMovementComponent::DrawDebugObjects()
{
	for (FGear Gear : Gears)
	{
		FVector WorldPosition = GetOwner()->GetTransform().TransformPosition(Gear.RelativeLocation);
		
		if (Gear.IsBogey)
		{
			if (Gear.HasWeightOnWheel)
			{
				DrawDebugPoint(GetWorld(), WorldPosition, 8, FColor::Red, false);
				DrawDebugLine(GetWorld(), WorldPosition, WorldPosition + Gear.Force, FColor::Red, false, -1, 0, 3);
			}
			else
			{
				DrawDebugPoint(GetWorld(), WorldPosition, 8, FColor::Green, false);
				
			}
		}
		else
		{
			DrawDebugPoint(GetWorld(), WorldPosition, 8, FColor(128,128,128), false);
		}
	}


	//for (int i = 0; i < GroundReactions->GetNumGearUnits(); i++)
	//{
	//	JSBSim::FGLGear* Gear = GroundReactions->GetGearUnit(i);

	//	if (Gear->IsBogey())
	//	{
	//		JSBSim::FGColumnVector3 BodyLocation = Gear->GetBodyLocation();

	//		double XOffset = BodyLocation(1);
	//		double YOffset = BodyLocation(2);
	//		double ZOffset = -BodyLocation(3);

	//		FVector WorldPosition = GetOwner()->GetTransform().TransformPosition(FVector(XOffset, YOffset, ZOffset) * FEET_TO_METER * 100);
	//		DrawDebugPoint(GetWorld(), WorldPosition, 8, FColor::Red, false);
	//	}
	//	
	//}
}


/////////// In-Editor Specific
#if WITH_EDITOR
void UJSBSimMovementComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_AircraftModel = GET_MEMBER_NAME_CHECKED(UJSBSimMovementComponent, AircraftModel);
	if (PropertyChangedEvent.Property != nullptr && PropertyChangedEvent.Property->GetFName() == NAME_AircraftModel)
	{
		// Load the aircraft, and make sure we recreate the Component properties for this new one. 
		LoadAircraft(true);
	}

	static const FName NAME_StructuralFrameOrigin = GET_MEMBER_NAME_CHECKED(UJSBSimMovementComponent, StructuralFrameOrigin);
	if (PropertyChangedEvent.Property != nullptr && PropertyChangedEvent.MemberProperty->GetFName() == NAME_StructuralFrameOrigin)
	{
		UpdateLocalTransforms();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UJSBSimMovementComponent::PrepareModelForCompVisualizer()
{
	if (!IsReadyForCompVisualizer)
	{
		if (Exec == nullptr || !AircraftModel.Compare(FString(Exec->GetModelName().c_str()), ESearchCase::IgnoreCase))
		{
			LoadAircraft(false);
			PrepareJSBSim();
			IsReadyForCompVisualizer = true;
		}
	}
}

#endif
