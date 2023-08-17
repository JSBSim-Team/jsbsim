// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// UE treats warning as errors. JSBSim has some warnings in its include files, so if we don't catch them inside this push/pop pragma, we won't be able to build...
// FGOutputType.h(151): warning C4263: 'bool JSBSim::FGOutputType::Run(void)': member function does not override any base class virtual member function
// FGOutputType.h(215): warning C4264: 'bool JSBSim::FGModel::Run(bool)': no override available for virtual member function from base 'JSBSim::FGModel'; function is hidden --- And others
// compiler.h(58): warning C4005: 'DEPRECATED': macro redefinition with UE_5.0\Engine\Source\Runtime\Core\Public\Windows\WindowsPlatformCompilerPreSetup.h(55): note: see previous definition of 'DEPRECATED'
// FGXMLElement.h(369): error C4458: declaration of 'name' hides class member
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
#endif

#include "models/FGInertial.h"
#include "math/FGLocation.h"
#include "math/FGColumnVector3.h"

#ifdef _MSC_VER
#pragma warning( pop )
#elif defined(__clang__)
#pragma clang diagnostic pop
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
