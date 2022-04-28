// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FDMTypes.generated.h"


#define FEET_TO_METER 0.3048
#define METER_TO_FEET 3.2808398950131233595800524934383

#define FEET_TO_CENTIMETER 30.48
#define INCH_TO_CENTIMETER 2.54

#define FEET_PER_SEC_TO_KNOT 0.592484
#define KNOT_TO_FEET_PER_SEC 1.68781

USTRUCT(BlueprintType)
struct FTank
{
	GENERATED_BODY()

	FTank()
	{
	}

	// Editable in Initial Conditions
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		double FuelDensityPoundsPerGallon = 6.6;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		double ContentGallons = 300;


	// Basic Properties
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		double CapacityGallons = 300;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		double FillPercentage = 100;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		double TemperatureCelcius = 0;

	// Possible Other Functions - Fill, Drain... 

	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += FString::Printf(TEXT("      Content %.2f / %.2f gal [%.1f %%], Temp %.1f C, Density %.2f ppg"), ContentGallons, CapacityGallons, FillPercentage, TemperatureCelcius, FuelDensityPoundsPerGallon) + LINE_TERMINATOR;
		return DebugMessage;
	}
};

USTRUCT(BlueprintType)
struct FGear
{
  GENERATED_BODY()

	FGear()
	{
	}

	/**
	 * 1 = Down, 0 = up
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double NormalizedPosition = 1;

    /*
    * Doesn't exist in JSBSim, but need to be set in Editor in case you want to do separate gear animations 
    */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool IsFrontBogey = false;

    /*
    * Doesn't exist in JSBSim, but need to be set in Editor in case you want to do separate gear animations
    */
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
	bool IsRearBogey = false;


	// Basic Properties from JSBSim - Read Only
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString Name = "";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool IsBogey = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool HasWeightOnWheel = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double WheelRollLinearVelocityMetersPerSec = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool IsUp = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool IsDown = true;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector RelativeLocation = FVector(FVector::ZeroVector);

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector Force = FVector(FVector::ZeroVector);

	// Possible Other Functions - Steering, Compression... 

	FString GetDebugMessage()
	{
		FString DebugMessage;
		FString UpDownState("I"); 
		if (NormalizedPosition == 0) UpDownState = FString("U");
		if (NormalizedPosition == 1) UpDownState = FString("D");
		DebugMessage += FString::Printf(TEXT("      NormPosition %.2f [%s]    WOW %d    RollLinVel %.1f    Force %.1f"), NormalizedPosition, *UpDownState, HasWeightOnWheel, WheelRollLinearVelocityMetersPerSec, Force.Length()) + LINE_TERMINATOR;
		return DebugMessage;
	}

};

UENUM(BlueprintType)
enum class EEngineType : uint8 {
	Unknown,
	Rocket,
	Piston,
	Turbine,
	Turboprop,
	Electric
};

USTRUCT(BlueprintType)
struct FEngineCommand
{
  GENERATED_BODY()

	FEngineCommand()
	{
	}

	// Common Engine Commands

    /* Normalized [0..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double Throttle = 0.0f;
	
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double Mixture = 0.0f;
	
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Starter = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Running = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double PropellerAdvance = 0.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool PropellerFeather = false;

	// Piston Engine Commands
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int Magnetos = 0;

	// Turbine Engine Commands
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Augmentation = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Injection = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Ignition = 0;
	
	// Turbine & TurboPropeller Engine Commands
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Reverse = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool CutOff = false;
	
	// TurboPropeller Engine Commands
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool GeneratorPower = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Condition = false;


	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += FString::Printf(TEXT("      Starter %d    Mixture %.2f    Running %d    CutOff %d  ---- Throttle %f  "), Starter, Mixture, Running, CutOff, Throttle ) + LINE_TERMINATOR;
		return DebugMessage;
	}

};


USTRUCT(BlueprintType)
struct FEngineState
{
  GENERATED_BODY()

	FEngineState()
	{
	}

	// Type
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	EEngineType EngineType = EEngineType::Turbine;

	// Common Engine States
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Starter = false;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Running = false;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double Thrust = 0;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double EngineRPM = 0;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double N1 = 0; // Turbine
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	double N2 = 0; // Turbine
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Augmentation = false; // Turbine
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Reversed = false; // Turbine + TurboProp
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Injection = false; // Turbine
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool CutOff = false; // Turbine + TurboProp
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	int Ignition = 0; // Turbine + 
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool GeneratorPower = false; // TurboProp
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool Condition = false; // TurboProp

	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += FString::Printf(TEXT("      Starter %d Ignition %d Running %d EngineRPM %.1f Thrust %.1f  "), Starter, Ignition, Running, EngineRPM, Thrust) + LINE_TERMINATOR;

		if (EngineType == EEngineType::Turbine)
		{
			DebugMessage += FString::Printf(TEXT("                  N1 %.2f N2 %.2f CutOff %d Augmentation %d Reversed %d Injection %d Ignition %d"), N1, N2, CutOff, Augmentation, Reversed, Injection, Ignition) + LINE_TERMINATOR;
		}
		
		return DebugMessage;
	}
};

USTRUCT(BlueprintType)
struct FFlightControlCommands
{
  GENERATED_BODY()

	FFlightControlCommands()
	{
	}

	// Basics

    /* Normalized [-1..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double Aileron = 0;
    /* Normalized [-1..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double Elevator = 0;
     /* Normalized [-1..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double Rudder = 0;

    /* Normalized [-1..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double YawTrim = 0;
    /* Normalized [-1..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double PitchTrim = 0;
    /* Normalized [-1..1] value expected */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
	double RollTrim = 0;

	// Wheels

    /* Normalized [-1..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double Steer = 0; // == Rudder??
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double LeftBrake = 0;
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double RightBrake = 0;
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double CenterBrake = 0;
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double ParkingBrake = 0;
    // 0 for up, 1 for down.
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double GearDown = 1;

	// Wings

    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double Flap = 0;
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double SpeedBrake = 0;
    /* Normalized [0..1] value expected */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
    double Spoiler = 0;

	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += TEXT("Flight Control Commands :"); DebugMessage += LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Flight : Elevator %.3f   Aileron    %.3f  Rudder  %.3f  YawTrim %.3f PitchTrim %.3f  RollTrim %.3f"), Elevator, Aileron, Rudder, YawTrim, PitchTrim, RollTrim) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Brakes : Left     %.3f   Right      %.3f  Center  %.3f  Parking %.3f"), LeftBrake, RightBrake, CenterBrake, ParkingBrake) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Wheels : Steer    %.3f   GearDown   %.3f"), Steer, GearDown) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Wings  : Flap     %.3f   SpeedBrake %.3f  Spoiler %.3f"), Flap, SpeedBrake, Spoiler, ParkingBrake) + LINE_TERMINATOR;

		return DebugMessage;
	}
};



USTRUCT(BlueprintType)
struct FAircraftState
{
  GENERATED_BODY()

	FAircraftState()
	{
	}

	// Articulated Parts State in degrees
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double ElevatorPosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double LeftAileronPosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double RightAileronPosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double RudderPosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double FlapPosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double SpeedBrakePosition = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Articulations")
	double SpoilersPosition = 0;


	// Speed
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double CalibratedAirSpeedKts = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double GroundSpeedKts = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double TotalVelocityKts = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	FVector VelocityNEDfps = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double AltitudeASLFt = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double AltitudeAGLFt = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double AltitudeRateFtps = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Speed")
	double StallWarning = 0;
	
	
	// Transformation
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	FVector ECEFLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	double Latitude = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	double Longitude = 0; 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	FRotator LocalEulerAngles = FRotator::ZeroRotator;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	FVector EulerRates = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Transformation")
	FVector UEForwardHorizontal = FVector::ZeroVector;
	
	// Misc
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Misc")
	bool Crashed = false;

	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += TEXT("Aircraft State :"); DebugMessage += LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Elevator %.2f     Left Aileron %.2f     Right Aileron %.2f     Rudder   %.2f     (Degree)"), ElevatorPosition, LeftAileronPosition, RightAileronPosition) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Flap     %.2f     SpeedBrake   %.2f     Spoilers      %.2f"), FlapPosition, SpeedBrakePosition, SpoilersPosition) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        CAS      %.2f (kt)     GroundSpeed %.2f (kt)     VelocityNED %s (ft/s)"), CalibratedAirSpeedKts, GroundSpeedKts, *VelocityNEDfps.ToString()) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        AltitudeASL %.2f (ft)     AltitudeAGL %.2f (ft)     AltitudeRateFtps %.2f (ft/s)     StallWarning %d"), AltitudeASLFt, AltitudeAGLFt, AltitudeRateFtps, StallWarning) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        ECEFLocation %s      Latitude %.3f      Longitude %.3f"), *ECEFLocation.ToString(), Latitude, Longitude) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("        Yaw %.5f (%.5f)      Pitch %.5f (%.5f)     Roll %.5f (%.5f) (Degrees) "), LocalEulerAngles.Yaw, EulerRates.X, LocalEulerAngles.Pitch, EulerRates.Y, LocalEulerAngles.Roll, EulerRates.Z) + LINE_TERMINATOR;


		return DebugMessage;
	}

	void Reset()
	{
		ElevatorPosition = 0;
		LeftAileronPosition = 0;
		RightAileronPosition = 0;
		RudderPosition = 0;
		FlapPosition = 0;
		SpeedBrakePosition = 0;
		SpoilersPosition = 0;
	}
};
