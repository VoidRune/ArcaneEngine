#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include "ArcaneEngine/Graphics/ImGui/ImGuiRenderer.h"
#include "TransferFunctionEditor.h"
#include "UI/UserInterface.h"
#include "Core/CameraFP.h"
#include <glm/glm.hpp>

class VolumeRenderer : public RendererBase
{
public:
	VolumeRenderer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~VolumeRenderer();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	bool LoadDataset(std::string fileName);
	void ResizeCanvas(uint32_t width, uint32_t height);
	void UpdateDescriptorSets();
	void UpdateTransferAndExtinctionImages();

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;
	std::unique_ptr<Arc::ImGuiRenderer> m_ImGuiRenderer;
	std::unique_ptr<TransferFunctionEditor> m_TransferFunctionEditor;
	std::unique_ptr<UserInterface> m_UserInterface;
	ImTextureID m_ImGuiDisplayImage;
	ImTextureID m_ImGuiTransferImage;
	ImVec2 m_ImGuiCanvasSize;

	std::unique_ptr<CameraFP> m_Camera;
	bool m_IsEvenFrame = false;
	bool m_RenderWithGUI = true;
	bool m_AccumulateFrames = true;
	struct GlobalFrameData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 inverseProjection;
		glm::mat4 inverseView;
		glm::vec4 cameraPosition;
		glm::vec4 cameraDirection;
		uint32_t frameIndex = 0;
		int32_t bounceLimit = 16;
		float extinction = 250.0f;
		float anisotropy = 0.2f;
		alignas(16) glm::vec3 backgroundColor = { 1, 1, 1 };
		float enableEnvironment = 0.0f;
		alignas(16) glm::ivec4 debugDraw = { 0, 1, 1, 1 };
	} globalFrameData;

	std::unique_ptr<Arc::GpuBufferArray> m_GlobalDataBuffer;
	std::unique_ptr<Arc::DescriptorSetArray> m_GlobalDataDescSet;

	std::string m_DatasetDirectory = "res/Datasets/";
	std::vector<std::string> m_Files;
	std::string m_SelectedDataset = "";
	glm::ivec3 m_DataSetSize = {0, 0, 0};
	uint32_t m_ExtinctionGridSize = 64;

	// Resources
	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::GpuImage> m_OutputImage;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage1;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage2;
	std::unique_ptr<Arc::GpuImage> m_DatasetImage;
	std::unique_ptr<Arc::GpuImage> m_TransferFunctionImage;
	std::unique_ptr<Arc::GpuImage> m_MaxExtinctionImage;

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