// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FDMTypes.generated.h"


#define FEET_TO_METER 0.3048
#define METER_TO_FEET 3.2808398950131233595800524934383

#define FEET_TO_CENTIMETER 30.48
#define INCH_TO_CENTIMETER 2.54


USTRUCT(BlueprintType)
struct FTank
{
	GENERATED_USTRUCT_BODY()

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
};

USTRUCT(BlueprintType)
struct FGear
{
	GENERATED_USTRUCT_BODY()

	FGear()
	{
	}

	/**
	 * 1 = Down, 0 = up
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		double NormalizedPosition = 1;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool IsFrontBogey = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool IsRearBogey = false;


	// Basic Properties - Non Editable
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FString Name = "";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool IsBogey = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool HasWeightOnWheel = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		double WheelRollLinearVelocity_mps = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool IsUp = false;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		bool IsDown = true;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FVector RelativeLocation = FVector(FVector::ZeroVector);

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
		FVector Force = FVector(FVector::ZeroVector);

	// Possible Other Functions - Steering, Compression... 
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
	GENERATED_USTRUCT_BODY()

	FEngineCommand()
	{
	}

	// Common Engine Commands
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	double Throttle = 0.0f;
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
		DebugMessage += FString::Printf(TEXT("      Starter %d Running %d Throttle %f Mixture %f CutOff %d"), Starter, Running, Throttle, Mixture, CutOff) + LINE_TERMINATOR;
		return DebugMessage;
	}

};


USTRUCT(BlueprintType)
struct FEngineState
{
	GENERATED_USTRUCT_BODY()

	FEngineState()
	{
	}
	

	// Type
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	EEngineType EngineType;

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
		DebugMessage += FString::Printf(TEXT("      Starter %d Ignition %d Running %d EngineRPM %f Thrust %f  "), Starter, Ignition, Running, EngineRPM, Thrust) + LINE_TERMINATOR;

		if (EngineType == EEngineType::Turbine)
		{
			DebugMessage += FString::Printf(TEXT("      N1 %f N2 %f CutOff %d Augmentation %d Reversed %d Injection %d Ignition %d"), N1, N2, CutOff, Augmentation, Reversed, Injection, Ignition) + LINE_TERMINATOR;
		}
		
		return DebugMessage;
	}
};

USTRUCT(BlueprintType)
struct FFlightControlCommands
{
	GENERATED_USTRUCT_BODY()

	FFlightControlCommands()
	{
	}

	// Basics
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Aileron;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Elevator;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Rudder;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double YawTrim;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double PitchTrim;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double RollTrim;

	// Wheels
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Steer; // == Rudder??
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double LeftBrake;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double RightBrake;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double CenterBrake;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double ParkingBrake;
	// 0 for up, 1 for down.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double GearDown = 1;

	// Wings
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Flap;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double SpeedBrake;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Commands")
		double Spoiler;
	




	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += TEXT("Flight Control Commands :"); DebugMessage += LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Basics : Elevator %.3f   Aileron    %.3f  Rudder  %.3f  YawTrim %.3f PitchTrim %.3f  RollTrim %.3f"), Elevator, Aileron, Rudder, YawTrim, PitchTrim, RollTrim) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Brakes : Left     %.3f   Right      %.3f  Center  %.3f  Parking %.3f"), LeftBrake, RightBrake, CenterBrake, ParkingBrake) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Wheels : Steer    %.3f   GearDown   %.3f  Center  %.3f  Parking %.3f"), Steer, GearDown) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Wings  : Flap     %.3f   SpeedBrake %.3f  Spoiler %.3f"), Flap, SpeedBrake, Spoiler, ParkingBrake) + LINE_TERMINATOR;

		return DebugMessage;
	}
};



USTRUCT(BlueprintType)
struct FAircraftState
{
	GENERATED_USTRUCT_BODY()

	FAircraftState()
	{
	}

	// Articulated Parts State in degrees
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double ElevatorPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double LeftAileronPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double RightAileronPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double RudderPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double FlapPosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double SpeedBrakePosition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Output|Articulations")
	double SpoilersPosition;

	FString GetDebugMessage()
	{
		FString DebugMessage;
		DebugMessage += TEXT("Aircraft State :"); DebugMessage += LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Elevator %f Left Aileron %f  Right Aileron %f"), ElevatorPosition, LeftAileronPosition, RightAileronPosition) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Rudder   %f "), RudderPosition) + LINE_TERMINATOR;
		DebugMessage += FString::Printf(TEXT("  Flap     %f SpeedBrake   %f  Spoilers      %f"), FlapPosition, SpeedBrakePosition, SpoilersPosition) + LINE_TERMINATOR;

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
