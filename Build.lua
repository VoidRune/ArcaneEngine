-- premake5.lua
workspace "ArcaneEngine"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Application"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

	flags
	{
		"MultiProcessorCompile"
	}

include "ArcaneEngine/Build-Engine.lua"
include "Application/Build-App.lua"