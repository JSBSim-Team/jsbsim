// Fill out your copyright notice in the Description page of Project Settings.


#include "JSBSimMovementCompVisualizer.h"
#include "JSBSimMovementComponent.h"


#pragma warning( push )
#pragma warning( disable : 4263 )
#pragma warning( disable : 4264 )
#pragma warning( disable : 4005 )

#include "math/FGColumnVector3.h"
#include "models/FGLGear.h"
#include "models/FGMassBalance.h"
#include "models/FGGroundReactions.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

#pragma warning( pop )

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
