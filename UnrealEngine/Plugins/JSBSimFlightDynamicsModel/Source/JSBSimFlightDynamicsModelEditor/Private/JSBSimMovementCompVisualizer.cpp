// Fill out your copyright notice in the Description page of Project Settings.


#include "JSBSimMovementCompVisualizer.h"
#include "JSBSimMovementComponent.h"
#include "CanvasTypes.h"

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

#include "math/FGColumnVector3.h"
#include "models/FGLGear.h"
#include "models/FGMassBalance.h"
#include "models/FGGroundReactions.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

#ifdef _MSC_VER
#pragma warning( pop )
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif

#define FEET_TO_METER 0.3048
#define METER_TO_FEET 3.2808398950131233595800524934383
#define INCH_TO_CENTIMETER 2.54


FJSBSimMovementCompVisualizer::FJSBSimMovementCompVisualizer()
{
}

FJSBSimMovementCompVisualizer::~FJSBSimMovementCompVisualizer()
{
}

void FJSBSimMovementCompVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UJSBSimMovementComponent* MovementComponent = Cast<UJSBSimMovementComponent>(Component);
	if (!MovementComponent)
	{
		return;
	}

	AActor* Owner = Component->GetOwner();

	// Make sure we are ready to visualize this component
	if (!MovementComponent->IsReadyForCompVisualizer)
	{
		// We must call PrepareModelForCompVisualizer(), but this is not a const method, and then can't be called using the MovementComponent pointer above. 
		// Trick - Grab an editable pointer to if from the owner... 
		UJSBSimMovementComponent* EditableMovementComponent = Cast<UJSBSimMovementComponent>(Owner->GetComponentByClass(UJSBSimMovementComponent::StaticClass()));
		if (EditableMovementComponent)
		{
			EditableMovementComponent->PrepareModelForCompVisualizer(); 
		}
	}
	
	// Draw visualization helpers
	if (Owner)
	{
		// Update Locations 
		StructuralFrameWorldLocation = Owner->GetTransform().TransformPosition(MovementComponent->StructuralFrameOrigin);
		CGWorldLocation = Owner->GetTransform().TransformPosition(MovementComponent->CGLocalPosition);
		EPWorldLocation = Owner->GetTransform().TransformPosition(MovementComponent->EPLocalPosition);
		VRPWorldLocation = Owner->GetTransform().TransformPosition(MovementComponent->VRPLocalPosition);
		 
		// UE Actor Origin - Blue
		PDI->DrawPoint(Owner->GetActorLocation(), FLinearColor::Blue, 10, SDPG_World);

		// Structural Frame Origin
		DrawCoordinateSystem(PDI, StructuralFrameWorldLocation, FRotator::ZeroRotator, 200, 0, 5);

		// Gravity Center
		PDI->DrawPoint(CGWorldLocation, FLinearColor::Yellow, 10, SDPG_World);

		// EyePoint 
		PDI->DrawPoint(EPWorldLocation, FLinearColor::Green, 10, SDPG_World);

		// Visual Reference Point
		PDI->DrawPoint(VRPWorldLocation, FLinearColor::Blue, 10, SDPG_World);
		 
		for (FGear Gear : MovementComponent->Gears)
		{
			FVector GearWorldLocation = Owner->GetTransform().TransformPosition(Gear.RelativeLocation);
			PDI->DrawPoint(GearWorldLocation, FLinearColor::Red, 5, SDPG_World);
		}
	}
}

void FJSBSimMovementCompVisualizer::DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	const UJSBSimMovementComponent* MovementComponent = Cast<UJSBSimMovementComponent>(Component);
	if (!MovementComponent)
	{
		return;
	}

	AActor* Owner = MovementComponent->GetOwner();

	FVector2D PixelLocation;

	// Aircraft name at Actor location
	if (View->ScreenToPixel(View->WorldToScreen(Owner->GetActorLocation()), PixelLocation))
	{
		FString AircraftName = FString::Printf(TEXT("%s - %s"), *MovementComponent->AircraftModel, *MovementComponent->GetAircraftScreenName());
		Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, *AircraftName, GEngine->GetSmallFont(), FLinearColor::Blue);
	}

    // Reference points names
	if (View->ScreenToPixel(View->WorldToScreen(CGWorldLocation), PixelLocation))
	{
		Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, TEXT("CG"), GEngine->GetSmallFont(), FLinearColor::Yellow);
	}

	if (View->ScreenToPixel(View->WorldToScreen(EPWorldLocation), PixelLocation))
	{
		Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, TEXT("EP"), GEngine->GetSmallFont(), FLinearColor::Green);
	}

	if (View->ScreenToPixel(View->WorldToScreen(VRPWorldLocation), PixelLocation))
	{
		Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, TEXT("VRP"), GEngine->GetSmallFont(), FLinearColor::Gray);
	}

    // Gear names
	for (FGear Gear : MovementComponent->Gears)
	{
		FVector GearWorldLocation = Owner->GetTransform().TransformPosition(Gear.RelativeLocation);
		if (View->ScreenToPixel(View->WorldToScreen(GearWorldLocation), PixelLocation))
		{
			Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, *(Gear.Name), GEngine->GetSmallFont(), FLinearColor::Red);
		}
	}
}
