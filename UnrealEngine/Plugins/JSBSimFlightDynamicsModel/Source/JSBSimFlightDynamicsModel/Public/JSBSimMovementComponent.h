// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <sstream>
#include "JSBSimModule.h"
#include "FDMTypes.h"
#include "JSBSimMovementComponent.generated.h"



// JSBSim Forward Declarations
namespace JSBSim {
	class FGFDMExec;
	class FGAtmosphere;
	class FGWinds;
	class FGFCS;
	class FGPropulsion;
	class FGMassBalance;
	class FGAerodynamics;
	class FGInertial;
	class FGAircraft;
	class FGPropagate;
	class FGAuxiliary;
	class FGOutput;
	class FGInitialCondition;
	class FGLocation;
	class FGAccelerations;
	class FGPropertyManager;
	class FGGroundReactions;
}

// UE Forward Declarations
class AGeoReferencingSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDelegateAircraftCrashed);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JSBSIMFLIGHTDYNAMICSMODEL_API UJSBSimMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UJSBSimMovementComponent();


	// Model Properties
 	
	/**
	 *	The Aircraft model name as expected in JSBSim (Name of folder/xml file)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
	FString AircraftModel = "";
	
	/**
	 * JSBSim does its computations in its own set or reference frames. All these frames are related to a "Structural Frame"
	 * https://jsbsim-team.github.io/jsbsim-reference-manual/mypages/user-manual-frames-of-reference/
	 * This structural frame is not aligned with the 3D modeling frame. This offset is meant to manually guess the offset between the 3D model and the internal logical model. 
	 * Activate debug mode, and tune this value to align reference points with your 3D model 
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
	FVector StructuralFrameOrigin;

	/**
	 * Display the reference points and debug text at runtime. 
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model")
	bool DrawDebug = true;


    /**
	 * When querying for the Above Ground Level, JSBSim can throw raycasts from several points, sometimes under the StructuralFrameOrigin. 
    *  By doing that, some of them can fail if they start below the ground. This value is a vertical offset added to each AGL Query to make sure we hit the ground. 
    *  (Aircraft geometry is of course ignored during the process - 15m should be sufficient for all kind of aircrafts - Issue #786)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model|Settings")
	float AGLThresholdMeters = 15;

	/**
	 * Center of Gravity Location in Actor local frame
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Model|Reference Points")
	FVector CGLocalPosition;

	/**
	 * JSBSim's Eye Position in Actor local frame
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Model|Reference Points")
	FVector EPLocalPosition;

	/**
	 * JSBSim's Visual Reference Point Position in Actor local frame
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Model|Reference Points")
	FVector VRPLocalPosition;



	// Initial Conditions Properties

	/**
	 * Control the behavior of initial Aircraft Trim. If true, an initial stall state will be computed. 
	 * If false, the aircraft will be considered active. Depending on its precise location on ground, you could see some damping. 
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Aircraft")
	bool StartOnGround = true;
	
	/**
	 * Gear Position on play
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Aircraft")
	bool bStartWithGearDown = true;

	/**
	 * Engine state at start
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Aircraft")
	bool bStartWithEngineRunning = true;
	
	/**
	 * Flaps Normalized Position on play [0..1]
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Aircraft", meta = (UIMin = "0", UIMax = "1.0", ClampMin = "0", ClampMax = "1.0"))
	double FlapPositionAtStart = 0;
	
	/**
	 * Calibrated Air Speed wanted on Play. (In knots)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Aircraft", meta = (UIMin = "0", UIMax = "600", ClampMin = "0"))
	double InitialCalibratedAirSpeedKts = 0;



	// Atmosphere Properties 

	/**
	 * Wind Heading Degrees
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Atmosphere", meta = (UIMin = "1", UIMax = "360", ClampMin = "1", ClampMax = "360"))
		int32 WindHeading = 0;

	/**
	* Wind Intensity knots
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Atmosphere", meta = (UIMin = "0", UIMax = "100", ClampMin = "0", ClampMax = "1000"))
		double WindIntensityKts = 0;

	/**
	 * If false, the atmosphere model will be the one from JSBSim. 
	 * If true, you can define it with custom properties
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Atmosphere")
	bool ControlFDMAtmosphere = false;
	
	/**
	 * Atmosphere temperature around the Aircraft (Celsius)
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Atmosphere", meta = (EditCondition = "ControlFDMAtmosphere", UIMin = "-80", ClampMin = "-273"))
	double TemperatureCelsius = 12.0f;

	/**
	 * Atmosphere pressure at Sea level (HectoPascals)
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Initial Conditions|Atmosphere", meta = (EditCondition = "ControlFDMAtmosphere", UIMin = "0", ClampMin = "0"))
	double PressureSeaLevelhPa = 1013.25;
	// TODO - Other ATM properties, maybe in a dedicated structure... 


	// Tanks Properties 

	UPROPERTY(BlueprintReadOnly, Editfixedsize, EditAnywhere, Category = "Model|Tanks")
	TArray<struct FTank> Tanks;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Model|Tanks")
	bool FuelFreeze = false;

	// Gear Properties 

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Editfixedsize, Category = "Model|Gears", meta=(TitleProperty = "{Name} Bogey = {IsBogey}"))
	TArray<struct FGear> Gears;

	// Engine Properties 
	
	UPROPERTY(Transient, BlueprintReadOnly, Editfixedsize, EditAnywhere, Category = "Model|Engines")
	TArray<struct FEngineCommand> EngineCommands;
	UPROPERTY(Transient, BlueprintReadOnly, Editfixedsize, EditAnywhere, Category = "Model|Engines")
	TArray<struct FEngineState> EngineStates;

	// Flight Control Commands and State
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	FFlightControlCommands Commands;

	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere, Category = "State")
	FAircraftState AircraftState;

  // Events
  UPROPERTY(VisibleAnywhere, BlueprintAssignable)
  FDelegateAircraftCrashed AircraftCrashed;


    // Functions

	/* Returns the full Aircraft name as set in the JSBSim definition file */ 
	FString GetAircraftScreenName() const;

    /* This function is used in different contexts : 
    *  - When the used changed the Aircraft Model string - In that case, we call it with ResetToDefaultSettings to rebuild Engine, Tanks and Gears UE structures from the new aircraft
    *  - On begin play, in that case, we don't touch the UE structures because the user can have overriden some properties. */
	UFUNCTION(CallInEditor, DisplayName ="Reset Initial Conditions")
	void LoadAircraft(bool ResetToDefaultSettings = true);

    /* Query the ground for a contact point and normal - JSBSims uses a lot this function to query contacts */
    double GetAGLevel(const FVector& ECEFLocation, FVector& ECEFContactPoint, FVector& Normal);

	// ActorComponent overridables
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void BeginDestroy() override;

	/**Gets Catalog of all properties in Property Manger
  *   -Returns names of all properties that JSBSim created/loaded
  *   -Currently not very useful other than to see what exists
  *   -Returns a big list, probably should not call often  */
	UFUNCTION(BlueprintCallable, DisplayName = "Property Manager Get Catalog")
    void PropertyManagerNode(TArray<FString> & Catalog);

	/**Command Input & Output from Property Manger
  *   -Enter name of property, e.g. gear/unit/wheel-speed-fps
  *   -OutValue of blank/empty means property name does not exist.
  *   -InValue of blank/empty if you wish to only lookup a property value,
  *     otherwise you will override the system value!*/
	UFUNCTION(BlueprintCallable, DisplayName = "Command Console")
    void CommandConsole(FString Property, FString InValue, FString & OutValue);

	/**Command Inputs & Outputs in Batch to Property Manger
  *   -Enter name of property, e.g. gear/unit/wheel-speed-fps
  *   -OutValue of blank/empty means property name does not exist.
  *   -InValue of blank/empty if you wish to only lookup a property value,
  *     otherwise you will override the system value! */
	UFUNCTION(BlueprintCallable, DisplayName = "Command Console Batch")
    void CommandConsoleBatch(TArray<FString> Property, TArray<FString> InValue, TArray<FString>& OutValue);
	/**
	 * Set environmental wind parameters
	 *
	 */
	UFUNCTION(BlueprintCallable, DisplayName = "Set Winds")
	void SetWind(FSimpleWindState WindState);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	UPROPERTY()
	AGeoReferencingSystem* GeoReferencingSystem;

	void OnRegister() override;

protected:
	// JSBSim Objects - TODO - Hide in an JSBSimInternals Structure to allow adding a public dependency without requiring JSBSim Includes
	JSBSim::FGFDMExec* Exec = nullptr;
	
	std::shared_ptr<JSBSim::FGAtmosphere> Atmosphere = nullptr;
	std::shared_ptr<JSBSim::FGWinds> Winds = nullptr;
	std::shared_ptr<JSBSim::FGFCS> FCS = nullptr;
	std::shared_ptr<JSBSim::FGMassBalance> MassBalance = nullptr;
	std::shared_ptr<JSBSim::FGPropulsion> Propulsion = nullptr;
	
	std::shared_ptr<JSBSim::FGAircraft> Aircraft = nullptr;
	std::shared_ptr<JSBSim::FGPropagate> Propagate = nullptr;
	std::shared_ptr<JSBSim::FGAuxiliary> Auxiliary = nullptr;
	std::shared_ptr<JSBSim::FGInertial> Inertial = nullptr;
	std::shared_ptr<JSBSim::FGAerodynamics> Aerodynamics = nullptr;

	std::shared_ptr<JSBSim::FGGroundReactions> GroundReactions = nullptr;
	std::shared_ptr<JSBSim::FGAccelerations> Accelerations = nullptr;
	
	std::shared_ptr<JSBSim::FGPropertyManager> PropertyManager = nullptr; // TODO ?? virer ?
	std::shared_ptr<JSBSim::FGInitialCondition> IC = nullptr;

	FTransform StructuralToActor;
	FTransform BodyToActor;

	bool JSBSimInitialized = false;
    bool TrimNeeded = true; // TODO ? Does false make sense ? we need to Trim the state based on the ICs... 
	bool Trimmed = false;
	bool AircraftLoaded = false;
	float simDtime = 0.f;
	float remainder = 0.f;
	int simloops = 0;
	
	double TickTime = 0;

	/////////// JSBSim Protected methods

	/**
	 * @brief Create and allocate all JSB Sim classes, without loading any model
	*/
	void InitializeJSBSim();

	/**
	 * @brief On play, take the defined initial conditions, and apply them to the model,
	 * This initialize all integrators and make the aircraft ready to fly
	*/
	void PrepareJSBSim();

	/**
	 * @brief Destroy all JSB Sim classes
	*/
	void DeInitializeJSBSim();

	/**
	 * @brief  Takes UE Status and commands, and forward them to JSBSim Model
	*/
	void CopyToJSBSim();

	/**
	 * @brief  Get the result state from JSBSim to update UE State.
	*/
	void CopyFromJSBSim();

private:
	AActor* Parent = nullptr;
  	FVector ECEFForwardHorizontal;

	/////////// JSBSim Private methods
	void DoTrim();

	void UpdateLocalTransforms();

	// Gears
	void InitGearDefaultProperties();
	void CopyGearPropertiesToJSBSim();
	void CopyGearPropertiesFromJSBSim();
	
	// Tanks
	void InitTankDefaultProperties();
	void CopyTankPropertiesToJSBSim();
	void CopyTankPropertiesFromJSBSim();

	// Engines
	void ApplyEnginesCommands();
	void InitEnginesCommandAndStates();
	void GetEnginesStates();


	//aircrafts
	void CrashedEvent();

	/////////// Logging and Debugging Methods
	void LogInitialization();


	/**
	 * @brief  Draw Onscreen debug text during play
	*/
	void DrawDebugMessage();
	void DrawDebugObjects();

	/////////// In-Editor Specific

#if WITH_EDITOR
public:
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * When some properties changes on this component, the Editor system creates a new component from the BP template, and update the corresponding properties
	 * It doesn't recreate the native objects, nor load the right JSBSim models. (There is no way to get notified of this re-init from within a component) 
	 * But in order to display data in the visualizer, we must make sure that : 
	 *  - The JSBSim Aircraft has been loaded
	 *  - The Initial state has been computed from the IC (eg : Center of Gravity Location)
	 * This method is meant to be called from the visualizer to make sure the model is ready for being used in the visualizer. 
	*/
	void PrepareModelForCompVisualizer();

	bool IsReadyForCompVisualizer = false;

#endif

	
};
