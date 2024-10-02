workspace "ArcaneEngine"
   	architecture "x64"
   	configurations { "Debug", "Release", "Dist" }
   	startproject "Application"

	flags
	{
		"MultiProcessorCompile"
	}

include "ArcaneEngine/Build-Engine.lua"
include "Application/Build-App.lua"