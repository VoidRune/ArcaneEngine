project "ArcaneEngine"
   	kind "StaticLib"
   	language "C++"
   	cppdialect "C++20"
   	staticruntime "off"

	targetdir ("../bin/%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	objdir ("../build/%{cfg.system}-%{cfg.architecture}/%{prj.name}")
	--debugdir ("$(ProjectDir)")
	debugdir ("../bin/%{cfg.system}-%{cfg.architecture}/%{prj.name}")

   	files 
	{ 
		"src/**.h", 
		"src/**.cpp",
		"dependencies/imgui-master/**.h",
		"dependencies/imgui-master/**.cpp",
	}

   	includedirs
   	{
		"src",
		"$(VULKAN_SDK)/include",
		"dependencies/glfw/include",
		"dependencies/GameNetworkingSockets/include",
   	}

	links
	{
		"$(VULKAN_SDK)/lib/vulkan-1",
		"dependencies/glfw/lib-vc2022/glfw3",
	}

	libdirs
	{ 
		"$(VULKAN_SDK)/lib",
	}
	
	filter { "system:windows", "configurations:Debug" }	
      		links
      		{
          		"dependencies/GameNetworkingSockets/bin/Windows/Debug/GameNetworkingSockets.lib"
      		}

  	filter { "system:windows", "configurations:Release or configurations:Dist" }	
      		links
      		{
          		"dependencies/GameNetworkingSockets/bin/Windows/Release/GameNetworkingSockets.lib"
      		}

   	filter "system:windows"
       		systemversion "latest"
       		defines { }

   	filter "configurations:Debug"
		targetname "ArcaneEngineDebug"
       		defines { "DEBUG" }
       		runtime "Debug"
       		symbols "On"

   	filter "configurations:Release"
		targetname "ArcaneEngineRelease"
       		defines { "RELEASE" }
       		runtime "Release"
       		optimize "On"
       		symbols "On"

   	filter "configurations:Dist"
		targetname "ArcaneEngine"
       		defines { "DIST" }
       		runtime "Release"
       		optimize "On"
       		symbols "Off"