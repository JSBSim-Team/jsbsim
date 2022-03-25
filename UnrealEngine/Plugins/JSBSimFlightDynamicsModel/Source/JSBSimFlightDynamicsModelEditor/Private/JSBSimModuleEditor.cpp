// Copyright Epic Games, Inc. All Rights Reserved.


#include "JSBSimModuleEditor.h"
#include "UnrealEd.h"
#include "JSBSimMovementCompVisualizer.h"
#include "JSBSimMovementComponent.h"

//#include "Core.h"
//#include "Modules/ModuleManager.h"


#define LOCTEXT_NAMESPACE "FJSBSimModuleEditor"


void FJSBSimModuleEditor::StartupModule()
{
	if (GUnrealEd)
	{
		// Make a new instance of the visualizer
		TSharedPtr<FComponentVisualizer> Visualizer = MakeShareable(new FJSBSimMovementCompVisualizer);


		// Register it to our specific component class
		GUnrealEd->RegisterComponentVisualizer(UJSBSimMovementComponent::StaticClass()->GetFName(), Visualizer);
		Visualizer->OnRegister();
	}
}

void FJSBSimModuleEditor::ShutdownModule()
{
	if (GUnrealEd)
	{
		// Unregister when the module shuts down
		GUnrealEd->UnregisterComponentVisualizer(UJSBSimMovementComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJSBSimModuleEditor, JSBSimFlightDynamicsModelEditor)
