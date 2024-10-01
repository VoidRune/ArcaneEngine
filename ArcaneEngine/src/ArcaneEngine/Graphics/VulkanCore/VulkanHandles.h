#pragma once
#include <cstdint>

#define ARC_DEFINE_HANDLE(object) using object = void*;

#ifndef ARC_USE_64_BIT_PTR_DEFINES
	#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__) ) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
		#define ARC_USE_64_BIT_PTR_DEFINES 1
	#else
		#define ARC_USE_64_BIT_PTR_DEFINES 0
	#endif
#endif

#ifndef ARC_DEFINE_NON_DISPATCHABLE_HANDLE
	#if (ARC_USE_64_BIT_PTR_DEFINES==1)
		#define ARC_DEFINE_NON_DISPATCHABLE_HANDLE(object) using object = void*;
	#else
		#define ARC_DEFINE_NON_DISPATCHABLE_HANDLE(object) using object = uint64_t;
	#endif
#endif

namespace Arc
{
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(BufferHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(ImageHandle)
	ARC_DEFINE_HANDLE(InstanceHandle)
	ARC_DEFINE_HANDLE(PhysicalDeviceHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(DebugUtilsMessengerHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(SurfaceHandle)
	ARC_DEFINE_HANDLE(DeviceHandle)
	ARC_DEFINE_HANDLE(QueueHandle)
	ARC_DEFINE_HANDLE(AllocatorHandle)
	ARC_DEFINE_HANDLE(AllocationHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(SemaphoreHandle)
	ARC_DEFINE_HANDLE(CommandBufferHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(FenceHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(DeviceMemoryHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(EventHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(QueryPoolHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(BufferViewHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(ImageViewHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(ShaderModuleHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(PipelineCacheHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(PipelineLayoutHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(PipelineHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(RenderPassHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(DescriptorSetLayoutHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(SamplerHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(DescriptorSetHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(DescriptorPoolHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(FramebufferHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(CommandPoolHandle)
	ARC_DEFINE_NON_DISPATCHABLE_HANDLE(SwapchainHandle)
}