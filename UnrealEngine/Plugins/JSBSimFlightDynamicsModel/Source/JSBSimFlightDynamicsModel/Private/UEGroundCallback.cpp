// Fill out your copyright notice in the Description page of Project Settings.


#include "UEGroundCallback.h"
#include "JSBSimMovementComponent.h"

UEGroundCallback::UEGroundCallback(UJSBSimMovementComponent* InMovementComponent)
{
	MovementComponent = InMovementComponent;
}

UEGroundCallback::~UEGroundCallback()
{
}

double UEGroundCallback::GetAGLevel(const FGLocation& location, FGLocation& contact, FGColumnVector3& normal, FGColumnVector3& vel, FGColumnVector3& angularVel) const
{
	vel.InitMatrix();
	angularVel.InitMatrix();

	FVector ECEFLocation = FVector(location(1), location(2), location(3)) * FEET_TO_METER;
	FVector ECEFContactPoint;
	FVector Normal;
	double AGDistance = MovementComponent->GetAGLevel(ECEFLocation, ECEFContactPoint, Normal) * METER_TO_FEET;

	contact.SetEllipse(20925646.32546, 20855486.5951); // Important to set these values, otherwise the contact location is not valid, and the FGLocation::FGLocation(FGColumnVector3) don't do it!
	contact = FGColumnVector3(ECEFContactPoint.X, ECEFContactPoint.Y, ECEFContactPoint.Z)*METER_TO_FEET; // The = operators sets ECEF Coordinates
	normal = FGColumnVector3(Normal.X, Normal.Y, Normal.Z);
	return AGDistance;
}

double UEGroundCallback::GetAGLevel(double t, const FGLocation& location, FGLocation& contact, FGColumnVector3& normal, FGColumnVector3& vel, FGColumnVector3& angularVel) const
{
	// Don't care about time here... 
	return GetAGLevel(location, contact, normal, vel, angularVel);
}
