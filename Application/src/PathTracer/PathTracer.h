#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include "Core/CameraFP.h"
#include <glm/glm.hpp>

class PathTracer : public RendererBase
{
public:
	PathTracer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue);
	~PathTracer();

	void RenderFrame(float elapsedTime);
	void SwapchainResized(void* presentQueue);
	void RecompileShaders();
	void WaitForFrameEnd();

private:

	void CreateAccelerationStructure();
	void CreatePipelines();
	void CreateSamplers();
	void CreateImages();


	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;

		bool operator==(const Vertex& other) const noexcept {
			return pos == other.pos && normal == other.normal && uv == other.uv;
		}
	};
	struct VertexHasher {
		size_t operator()(const Vertex& v) const noexcept {
			size_t h1 = std::hash<float>{}(v.pos.x) ^ (std::hash<float>{}(v.pos.y) << 1) ^ (std::hash<float>{}(v.pos.z) << 2);
			size_t h2 = std::hash<float>{}(v.normal.x) ^ (std::hash<float>{}(v.normal.y) << 1) ^ (std::hash<float>{}(v.normal.z) << 2);
			size_t h3 = std::hash<float>{}(v.uv.x) ^ (std::hash<float>{}(v.uv.y) << 1);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
	bool LoadObjModel(std::string filePath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices);


	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;

	struct GlobalFrameData
	{
		glm::mat4 InverseView;
		glm::mat4 InverseProjection;
		uint32_t FrameIndex;
	} globalFrameData;

	std::unique_ptr<Arc::GpuBufferArray> m_GlobalDataBuffer;
	std::unique_ptr<CameraFP> m_Camera;
	bool m_IsEvenFrame = false;

	std::unique_ptr<Arc::GpuBuffer> m_VertexBuffer;
	std::unique_ptr<Arc::GpuBuffer> m_IndexBuffer;

	std::unique_ptr<Arc::Shader> m_RayGenShader;
	std::unique_ptr<Arc::Shader> m_RayMissShader;
	std::unique_ptr<Arc::Shader> m_RayClosestHitShader;
	std::unique_ptr<Arc::RayTracingPipeline> m_RayTracingPipeline;
	std::unique_ptr<Arc::AccelerationStructure> m_AccelerationStructure;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage1;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage2;
	std::unique_ptr<Arc::GpuImage> m_OutputImage;

	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::Shader> m_CompositeVertShader;
	std::unique_ptr<Arc::Shader> m_CompositeFragShader;
	std::unique_ptr<Arc::Pipeline> m_CompositePipeline;
};