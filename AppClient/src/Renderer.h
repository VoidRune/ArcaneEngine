#pragma once
#include "Window/Window.h"
#include "Window/Input.h"
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"

#include "Asset/AssetCache.h"
#include <glm.hpp>
#include <future>

class Renderer
{
public:

	Renderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue);
	~Renderer();

	struct FrameData
	{
		glm::mat4 View;
		glm::mat4 Projection;
		std::vector<Model> Models;
	};

	void RenderFrame(FrameData& frameData);
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

	Arc::GpuBuffer m_VertexBuffer;
	Arc::GpuBuffer m_IndexBuffer;

	/* Per frame descriptor */
	struct CameraFrameData
	{
		glm::mat4 view;
		glm::mat4 projection;
	} m_CameraFrameData;
	std::unique_ptr<Arc::InFlightGpuBuffer> m_CameraFrameDataBuffer;
	std::unique_ptr<Arc::InFlightDescriptorSet> m_GlobalDescriptor;
	std::unique_ptr<Arc::DescriptorSet> m_BindlessTexturesDescriptor;

	std::unique_ptr<Arc::Sampler> m_PointSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::DescriptorSet> m_PostprocessDescriptor;

	/* Shaders */
	std::unique_ptr<Arc::Shader> m_MeshVertexShader;
	std::unique_ptr<Arc::Shader> m_MeshFragmentShader;
	std::unique_ptr<Arc::Shader> m_PresentVertexShader;
	std::unique_ptr<Arc::Shader> m_PresentFragmentShader;

	/* Pipelines */
	std::unique_ptr<Arc::Pipeline> m_MeshPipeline;
	std::unique_ptr<Arc::Pipeline> m_PresentPipeline;

	/* Render pass data*/
	std::unique_ptr<Arc::Image> m_ColorAttachment;
	std::unique_ptr<Arc::Image> m_DepthAttachment;
	Arc::RenderPassProxy m_GeometryPassProxy;
	Arc::PresentPassProxy m_PresentPassProxy;
};