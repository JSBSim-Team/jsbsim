// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4263 )
#pragma warning( disable : 4264 )
#pragma warning( disable : 4005 )
#pragma warning( disable : 4458 )
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif

#include "models/FGInertial.h"
#include "math/FGLocation.h"
#include "math/FGColumnVector3.h"

#ifdef _MSC_VER
#pragma warning( pop )
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC pop
#endif

#include "CoreMinimal.h"


using namespace JSBSim;

class UJSBSimMovementComponent;

/**
 * JSBSim derived class called for ground queries. 
 * We'll delegate the ground queries to the UJSBSimMovementComponent because this one belongs to an Actor which has access to the world for doing the requests.
 * Using static GWorld is not recommended because there are different editor/runtime worlds depending on context... 
 */
class JSBSIMFLIGHTDYNAMICSMODEL_API UEGroundCallback : public JSBSim::FGGroundCallback
{
public:
	UEGroundCallback(UJSBSimMovementComponent* InMovementComponent/*, double semimajor, double semiminor*/);
	~UEGroundCallback();												  

	/** Compute the altitude above ground.
	  The altitude depends on time t and location l.
	  @param t simulation time
	  @param l location
	  @param contact Contact point location below the location l
	  @param normal Normal vector at the contact point
	  @param v Linear velocity at the contact point
	  @param w Angular velocity at the contact point
	  @return altitude above ground
   */
	double GetAGLevel(const FGLocation& location, FGLocation& contact, FGColumnVector3& normal, FGColumnVector3& vel, FGColumnVector3& angularVel) const override;


	/** Compute the altitude above ground.
	  The altitude depends on location l.
	  @param l location
	  @param contact Contact point location below the location l
	  @param normal Normal vector at the contact point
	  @param v Linear velocity at the contact point
	  @param w Angular velocity at the contact point
	  @return altitude above ground
   */
	double GetAGLevel(double t, const FGLocation& location, FGLocation& contact, FGColumnVector3& normal, FGColumnVector3& vel, FGColumnVector3& angularVel) const override;
private:
	UJSBSimMovementComponent* MovementComponent;
};
