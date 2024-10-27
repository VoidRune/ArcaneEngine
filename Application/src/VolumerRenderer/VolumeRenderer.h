#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"
#include "TransferFunctionEditor.h"
#include "Core/CameraFP.h"
#include <glm/glm.hpp>

class VolumeRenderer : RendererBase
{
public:
	VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~VolumeRenderer();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:
	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;
	std::unique_ptr<Arc::ImGuiRenderer> m_ImGuiRenderer;
	std::unique_ptr<TransferFunctionEditor> m_TransferFunctionEditor;
	ImTextureID m_ImGuiDisplayImage;

	std::unique_ptr<CameraFP> m_Camera;
	bool m_IsEvenFrame = false;
	struct GlobalFrameData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 inverseProjection;
		glm::mat4 inverseView;
		glm::vec4 cameraPosition;
		glm::vec4 cameraDirection;
		uint32_t frameIndex;
		uint32_t bounceLimit;
		float extinction;
		float anisotropy;
		glm::vec3 backgroundColor;
	} globalFrameData;

	std::unique_ptr<Arc::GpuBufferArray> m_GlobalDataBuffer;
	std::unique_ptr<Arc::DescriptorSetArray> m_GlobalDataDescSet;


	// Resources
	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::GpuImage> m_OutputImage;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage1;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage2;
	std::unique_ptr<Arc::GpuImage> m_DatasetImage;
	std::unique_ptr<Arc::GpuImage> m_TransferFunctionImage;

	// Compute Pass
	std::unique_ptr<Arc::Shader> m_VolumeShader;
	std::unique_ptr<Arc::ComputePipeline> m_VolumePipeline;
	std::unique_ptr<Arc::DescriptorSet> m_VolumeImageDescriptor1;
	std::unique_ptr<Arc::DescriptorSet> m_VolumeImageDescriptor2;
	// Present Pass
	std::unique_ptr<Arc::Shader> m_VertShader;
	std::unique_ptr<Arc::Shader> m_FragShader;
	std::unique_ptr<Arc::Pipeline> m_PresentPipeline;
	std::unique_ptr<Arc::DescriptorSet> m_PresentDescriptor;
};