#pragma once
#include "Window/Window.h"
#include "Window/Input.h"
#include "Graphics/Device.h"
#include "Graphics/PresentQueue.h"

#include "Asset/AssetCache.h"
#include "RenderFrameData.h"
#include <future>

class Renderer
{
public:

	Renderer(Arc::Window* window, Arc::Device* core, Arc::PresentQueue* presentQueue);
	~Renderer();

	void RenderFrame();
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

	void BindBindlessTexture(uint32_t arrayIndex, Arc::Image image);

	RenderFrameData& GetFrameRenderData() { return m_FrameRenderData[m_FrameRenderDataIndex]; }

	static Renderer* Get() { return s_Instance; }
private:

	void CompileShaders();
	void PreparePipelines();
	void BuildRenderGraph();

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	VkExtent2D m_WindowSize;
	std::future<void> m_RenderThreadFuture{};

	uint32_t m_FrameRenderDataIndex;
	std::vector<RenderFrameData> m_FrameRenderData;

	/* Per frame descriptor */
	struct CameraFrameData
	{
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 InvView;
		glm::mat4 InvProjection;
	} m_CameraFrameData;
	std::unique_ptr<Arc::GpuBufferSet> m_CameraFrameDataBuffer;
	std::unique_ptr<Arc::DescriptorSetArray> m_GlobalDescriptor;
	std::unique_ptr<Arc::DescriptorSet> m_BindlessTexturesDescriptor;

	uint32_t m_MaxTransforms = 4096;
	std::unique_ptr<Arc::GpuBufferSet> m_TransformMatricesBuffer;
	uint32_t m_MaxBoneMatrices = 4096;
	std::unique_ptr<Arc::GpuBufferSet> m_BoneMatricesBuffer;

	std::unique_ptr<Arc::Sampler> m_PointSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::DescriptorSet> m_PostprocessDescriptor;

	/* Shaders */
	std::unique_ptr<Arc::Shader> m_MeshVertexShader;
	std::unique_ptr<Arc::Shader> m_MeshFragmentShader;
	std::unique_ptr<Arc::Shader> m_DynamicMeshVertexShader;
	std::unique_ptr<Arc::Shader> m_PresentVertexShader;
	std::unique_ptr<Arc::Shader> m_PresentFragmentShader;

	/* Pipelines */
	std::unique_ptr<Arc::Pipeline> m_MeshPipeline;
	std::unique_ptr<Arc::Pipeline> m_DynamicMeshPipeline;
	std::unique_ptr<Arc::Pipeline> m_PresentPipeline;

	/* Render pass data*/
	std::unique_ptr<Arc::Image> m_ColorAttachment;
	std::unique_ptr<Arc::Image> m_DepthAttachment;
	Arc::RenderPassProxy m_GeometryPassProxy;
	Arc::PresentPassProxy m_PresentPassProxy;

	static Renderer* s_Instance;
};