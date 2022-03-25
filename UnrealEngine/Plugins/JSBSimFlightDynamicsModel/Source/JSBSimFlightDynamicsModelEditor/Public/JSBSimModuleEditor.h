// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FJSBSimModuleEditor : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

};
