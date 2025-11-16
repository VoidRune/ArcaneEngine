#pragma once
#include "RendererBase.h"
#include "ArcaneEngine/Window/Window.h"
#include "ArcaneEngine/Graphics/Device.h"
#include "ArcaneEngine/Graphics/PresentQueue.h"
#include "ArcaneEngine/Graphics/ResourceCache.h"
#include "ArcaneEngine/Graphics/RenderGraph.h"
#include "Core/CameraFP.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

	struct Model
	{
		Arc::GpuBuffer VertexBuffer;
		Arc::GpuBuffer IndexBuffer;
		Arc::BottomLevelAS BottomLevelAS;
		uint64_t VertexBufferDeviceAddress;
		uint64_t IndexBufferDeviceAddress;
	};
	void LoadModel(std::string filepath, Model* model);

	Arc::Window* m_Window;
	Arc::Device* m_Device;
	Arc::PresentQueue* m_PresentQueue;
	Arc::ResourceCache* m_ResourceCache;
	Arc::RenderGraph* m_RenderGraph;

	struct GlobalFrameData
	{
		glm::vec4 CameraPosition;
		glm::vec4 CameraForward;
		glm::vec4 CameraRight;
		glm::vec4 CameraUp;
		float TanHalfFov = 0;
		float AspectRatio = 0;
		float Aperture = 0;
		float FocusDistance = 0;
		uint32_t FrameIndex = 0;
	} globalFrameData;

	struct MeshInfo
	{
		uint64_t VertexBufferDeviceAddress;
		uint64_t IndexBufferDeviceAddress;
		glm::vec4 Color = glm::vec4(1);
		glm::vec4 Emission = glm::vec4(0);
		glm::vec4 Smoothness = glm::vec4(0);
	};
	void AddInstance(std::vector<MeshInfo>& meshInfos, Model* model, glm::mat4 transform, MeshInfo meshInfo);

	std::unique_ptr<Arc::GpuBuffer> m_MeshInfoBuffer;
	std::unique_ptr<Arc::DescriptorSet> m_SceneDescriptorSet;
	std::unique_ptr<Arc::GpuBufferArray> m_GlobalDataBuffer;
	std::unique_ptr<CameraFP> m_Camera;
	bool m_IsEvenFrame = false;

	std::unique_ptr<Arc::Shader> m_RayGenShader;
	std::unique_ptr<Arc::Shader> m_RayMissShader;
	std::unique_ptr<Arc::Shader> m_RayClosestHitShader;
	std::unique_ptr<Arc::RayTracingPipeline> m_RayTracingPipeline;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage1;
	std::unique_ptr<Arc::GpuImage> m_AccumulationImage2;
	std::unique_ptr<Arc::GpuImage> m_OutputImage;

	std::unique_ptr<Model> m_Plane;
	std::unique_ptr<Model> m_Dragon;
	std::unique_ptr<Arc::TopLevelAS> m_Scene;

	std::unique_ptr<Arc::Sampler> m_NearestSampler;
	std::unique_ptr<Arc::Sampler> m_LinearSampler;
	std::unique_ptr<Arc::Shader> m_CompositeVertShader;
	std::unique_ptr<Arc::Shader> m_CompositeFragShader;
	std::unique_ptr<Arc::Pipeline> m_CompositePipeline;
};