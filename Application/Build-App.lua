project "Application"
   	kind "ConsoleApp"
	--kind "WindowedApp"
   	language "C++"
   	cppdialect "C++20"
   	staticruntime "off"

	targetdir ("../bin/%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	objdir ("../build/%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	--debugdir ("$(ProjectDir)")
	debugdir ("../bin/%{cfg.system}-%{cfg.architecture}/%{prj.name}")

	dependson 
	{
		"ArcaneEngine",
	}

   	files 
	{ 
		"src/**.h", 
		"src/**.cpp",
		"dependencies/stb/**.h",
		"dependencies/stb/**.cpp",
	}

   	includedirs
   	{
		"src",
		"dependencies",


		"../ArcaneEngine/src",		
		"dependencies/glm",
   	}

   	links
   	{
      		"ArcaneEngine",
   	}


   	filter "system:windows"
       		systemversion "latest"
       		defines { "WINDOWS" }

   	filter "configurations:Debug"
		targetname "ApplicationDebug"
       		defines { "DEBUG" }
       		runtime "Debug"
       		symbols "On"

   	filter "configurations:Release"
		targetname "ApplicationRelease"
       		defines { "RELEASE" }
       		runtime "Release"
       		optimize "On"
       		symbols "On"

   	filter "configurations:Dist"
		targetname "Application"
       		defines { "DIST" }
       		runtime "Release"
       		optimize "On"
       		symbols "Off"