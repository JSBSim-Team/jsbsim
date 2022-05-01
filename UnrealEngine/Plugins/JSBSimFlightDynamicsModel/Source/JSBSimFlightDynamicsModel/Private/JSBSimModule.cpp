// Copyright Epic Games, Inc. All Rights Reserved.


#include "JSBSimModule.h"
#include "Core.h"
#include "Modules/ModuleManager.h"


#define LOCTEXT_NAMESPACE "FJSBSimModule"


void FJSBSimModule::StartupModule()
{
	
}

void FJSBSimModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FJSBSimModule, JSBSimFlightDynamicsModel)
DEFINE_LOG_CATEGORY(LogJSBSim);