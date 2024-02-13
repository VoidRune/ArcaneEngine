project "Application"
   	kind "ConsoleApp"
	--kind "WindowedApp"
   	language "C++"
   	cppdialect "C++20"
   	staticruntime "off"

	targetdir ("../bin/%{cfg.system}-%{cfg.architecture}")
	objdir ("../build/%{cfg.system}-%{cfg.architecture}")
	--debugdir ("$(ProjectDir)")
	debugdir ("../bin/%{cfg.system}-%{cfg.architecture}")

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
		"$(VULKAN_SDK)/include",
		"dependencies/glm",
   	}

   	links
   	{
      		"ArcaneEngine",
		"$(VULKAN_SDK)/lib/vulkan-1",
   	}

	libdirs
	{ 
		"$(VULKAN_SDK)/lib",
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