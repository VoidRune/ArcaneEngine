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
		"src/**.c",
		"dependencies/imgui-master/**.h",
		"dependencies/imgui-master/**.cpp",
		"dependencies/VulkanMemoryAllocator/**.h",
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
	}

   	includedirs
   	{
		"src",
		"$(VULKAN_SDK)/Include",
		"dependencies/glfw/include",
		"dependencies/VulkanMemoryAllocator",
		--"dependencies/GameNetworkingSockets/include",
   	}

	links
	{
		"$(VULKAN_SDK)/Lib/vulkan-1.lib",
		"dependencies/glfw/lib-vc2022/glfw3",
	}

	filter { "system:windows", "configurations:Debug" }	
      		links
      		{
			--"$(VULKAN_SDK)/Lib/glslangd.lib",
			--"$(VULKAN_SDK)/Lib/GenericCodeGend.lib",
			--"$(VULKAN_SDK)/Lib/glslang-default-resource-limitsd.lib",
			--"$(VULKAN_SDK)/Lib/MachineIndependentd.lib",
			--"$(VULKAN_SDK)/Lib/SPIRVd.lib",
			--"$(VULKAN_SDK)/Lib/SPIRV-Toolsd.lib",
			--"$(VULKAN_SDK)/Lib/SPIRV-Tools-optd.lib",
			--"$(VULKAN_SDK)/Lib/SPVRemapperd.lib",
			--"$(VULKAN_SDK)/Lib/OSDependentd.lib",
			"$(VULKAN_SDK)/Lib/shaderc_combinedd.lib",
			"$(VULKAN_SDK)/Lib/spirv-cross-cored.lib",
          		--"dependencies/GameNetworkingSockets/bin/Windows/Debug/GameNetworkingSockets.lib"
      		}

  	filter { "system:windows", "configurations:Release or configurations:Dist" }	
      		links
      		{
			--"$(VULKAN_SDK)/Lib/glslang.lib",
			--"$(VULKAN_SDK)/Lib/GenericCodeGen.lib",
			--"$(VULKAN_SDK)/Lib/glslang-default-resource-limits.lib",
			--"$(VULKAN_SDK)/Lib/MachineIndependent.lib",
			--"$(VULKAN_SDK)/Lib/SPIRV.lib",
			--"$(VULKAN_SDK)/Lib/SPIRV-Tools.lib",
			--"$(VULKAN_SDK)/Lib/SPIRV-Tools-opt.lib",
			--"$(VULKAN_SDK)/Lib/SPVRemapper.lib",
			--"$(VULKAN_SDK)/Lib/OSDependent.lib",
			"$(VULKAN_SDK)/Lib/shaderc_combined.lib",
			"$(VULKAN_SDK)/Lib/spirv-cross-core.lib",
          		--"dependencies/GameNetworkingSockets/bin/Windows/Release/GameNetworkingSockets.lib"
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