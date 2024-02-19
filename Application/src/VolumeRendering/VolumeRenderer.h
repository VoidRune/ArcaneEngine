#pragma once
#include "Window/Window.h"
#include "Window/Input.h"
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"

#include "CameraFPS.h"
#include "BaseRenderer.h"
#include <glm.hpp>
#include <future>

class VolumeRenderer : public BaseRenderer
{
public:
	VolumeRenderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue);
	~VolumeRenderer();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	void CompileShaders();
	void PreparePipelines();
	void BuildRenderGraph();

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	VkExtent2D m_WindowSize;
	float m_LastTime;
	std::unique_ptr<CameraFPS> m_Camera;
	std::future<void> m_RenderThreadFuture{};

	/* Per frame descriptor */
	struct CameraFrameData
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 inverseProjection;
		glm::mat4 inverseView;
		glm::vec4 cameraPos;
		glm::vec4 cameraDir;
		uint32_t frameIndex = 0;
		int32_t sampleCount = 32;
		int32_t lightSampleCount = 16;

		alignas(16) glm::vec3 backgroundColor = { 1.0, 1.0, 1.0 };
		float densityLimitMin = 0.1;
		float densityLimitMax = 1.0;
		float absorptionCoefficient = 4.0;
		float lightMultiplier = 1.0;
	} m_CameraFrameData;
	std::unique_ptr<Arc::InFlightGpuBuffer> m_CameraFrameDataBuffer;
	std::unique_ptr<Arc::InFlightDescriptorSet> m_GlobalDescriptor;
	std::unique_ptr<Arc::DescriptorSet> m_BindlessTexturesDescriptor;

	/* Shaders */
	std::unique_ptr<Arc::Shader> m_VolumeComputeShader;
	std::unique_ptr<Arc::Shader> m_PresentVertexShader;
	std::unique_ptr<Arc::Shader> m_PresentFragmentShader;

	/* ImGui data */
	struct DensityPoint
	{
		float location;
		float density;
		glm::vec3 color;
	};
	std::vector<uint8_t> m_DensityRemap;
	std::unique_ptr<Arc::Image> m_ColorGradientImage;
	std::unique_ptr<Arc::Image> m_DensityImage;
	VkDescriptorSet m_ImGuiDset;

	std::unique_ptr<Arc::Sampler> m_PointSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::DescriptorSet> m_PostprocessDescriptor;
	std::unique_ptr<Arc::DescriptorSet> m_ComputeDescriptor;

	/* Pipelines */
	std::unique_ptr<Arc::ComputePipeline> m_ComputePipeline;
	std::unique_ptr<Arc::Pipeline> m_PresentPipeline;

	/* Render pass data*/
	std::unique_ptr<Arc::Image> m_AccumulationImage;
	std::unique_ptr<Arc::Image> m_ComputeAttachment;
	Arc::ComputePassProxy m_ComputePassProxy;
	Arc::PresentPassProxy m_PresentPassProxy;
};