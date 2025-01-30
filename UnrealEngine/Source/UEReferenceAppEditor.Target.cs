// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UEReferenceAppEditorTarget : TargetRules
{
	public UEReferenceAppEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;

		#if UE_5_1_OR_LATER		
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		#endif

		ExtraModuleNames.AddRange( new string[] { "UEReferenceApp" } );
	}
}
