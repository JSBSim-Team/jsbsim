// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UEReferenceAppTarget : TargetRules
{
	public UEReferenceAppTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;

		#if UE_5_1_OR_LATER		
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		#endif

		ExtraModuleNames.AddRange( new string[] { "UEReferenceApp" } );
	}
}
