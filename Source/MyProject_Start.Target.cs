// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;



public class MyProject_StartTarget : TargetRules
{
	public MyProject_StartTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6; // 🔥 업그레이드
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7; // 🔥 최신
        ExtraModuleNames.Add("MyProject_Start");
	}


}


