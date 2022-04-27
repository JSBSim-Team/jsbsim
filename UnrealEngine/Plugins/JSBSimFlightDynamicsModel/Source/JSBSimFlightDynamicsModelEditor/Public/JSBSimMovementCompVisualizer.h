// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComponentVisualizer.h"

/**
 * 
 */
class JSBSIMFLIGHTDYNAMICSMODELEDITOR_API FJSBSimMovementCompVisualizer : public FComponentVisualizer
{
public:
	FJSBSimMovementCompVisualizer();
	~FJSBSimMovementCompVisualizer();

	void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;

private: 

	FVector StructuralFrameWorldLocation;
	FVector CGWorldLocation;
	FVector EPWorldLocation;
	FVector VRPWorldLocation;
};
