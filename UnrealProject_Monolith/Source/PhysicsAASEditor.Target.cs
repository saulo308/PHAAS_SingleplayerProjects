// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class PhysicsAASEditorTarget : TargetRules
{
	public PhysicsAASEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		//bOverrideBuildEnvironment = true;
		//bUseStaticCRT = true;
		ExtraModuleNames.AddRange( new string[] { "PhysicsAAS" } );
	}
}
