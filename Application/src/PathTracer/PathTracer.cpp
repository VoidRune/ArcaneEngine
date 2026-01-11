#include "PathTracer.h"
#include "ArcaneEngine/Window/Input.h"
#include "ArcaneEngine/Graphics/ShaderCompiler.h"
#include "ArcaneEngine/Core/Log.h"
#include "stb/stb_image_write.h"
#include "stb/stb_image.h"
#include "tiny_obj_loader/tiny_obj_loader.h"
#include <thread>

using namespace glm;

PathTracer::PathTracer(Arc::Window* window, Arc::Device* device, Arc::PresentQueue* presentQueue)
{
	m_Window = window;
	m_Device = device;
	m_PresentQueue = presentQueue;
	m_ResourceCache = m_Device->GetResourceCache();
	m_RenderGraph = m_Device->GetRenderGraph();

	m_Camera = std::make_unique<CameraFP>(m_Window);
	m_Camera->Position = glm::vec3(0, 0.5f, -1.5f);
	m_Camera->MovementSpeed = 1.0f;

	CreateSamplers();
	CreateAccelerationStructure();
	CreatePipelines();
	CreateImages();

	m_GlobalDataBuffer = std::make_unique<Arc::GpuBufferArray>();
	m_ResourceCache->CreateGpuBufferArray(m_GlobalDataBuffer.get(), Arc::GpuBufferDesc{
		.Size = sizeof(GlobalFrameData),
		.UsageFlags = Arc::BufferUsage::UniformBuffer,
		.MemoryProperty = Arc::MemoryProperty::HostVisible,
		});
}

bool PathTracer::LoadObjModel(std::string filePath, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
{
	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig config;
	config.triangulate = true;
	config.mtl_search_path = "./";

	if (!reader.ParseFromFile(filePath, config)) {
		if (!reader.Error().empty()) {
			ARC_LOG_ERROR("TinyObjReader: {}", reader.Error());
		}
		return false;
	}

	if (!reader.Warning().empty()) {
		ARC_LOG_WARNING("TinyObjReader: {}", reader.Warning());
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();

	std::unordered_map<Vertex, uint32_t, VertexHasher> uniqueVertices;

	for (const auto& shape : shapes) {
		size_t indexOffset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
			int fv = shape.mesh.num_face_vertices[f];

			for (int v = 0; v < fv; ++v) {
				tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

				Vertex vertex{};

				vertex.pos = {
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]
				};

				if (idx.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * idx.normal_index + 0],
						attrib.normals[3 * idx.normal_index + 1],
						attrib.normals[3 * idx.normal_index + 2]				
					};
				}
				else {
					vertex.normal = { 0.0f, 0.0f, 0.0f };
				}

				if (idx.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * idx.texcoord_index + 0],
						1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]
					};
				}
				else {
					vertex.uv = { 0.0f, 0.0f };
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(outVertices.size());
					outVertices.push_back(vertex);
				}

				outIndices.push_back(uniqueVertices[vertex]);
			}

			indexOffset += fv;
		}
	}

	ARC_LOG("Loaded OBJ model: {} vertices, {} indices", outVertices.size(), outIndices.size());
	return true;
}

void PathTracer::LoadModel(std::string filepath, Model* model)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	if (!LoadObjModel(filepath.c_str(), vertices, indices)) {
		ARC_LOG_ERROR("Failed to load OBJ! {}", filepath.c_str());
		return;
	}

	m_ResourceCache->CreateGpuBuffer(&model->VertexBuffer, Arc::GpuBufferDesc{
		.Size = (uint32_t)(vertices.size() * sizeof(Vertex)),
		.UsageFlags = Arc::BufferUsage::StorageBuffer | Arc::BufferUsage::ShaderDeviceAddress | Arc::BufferUsage::AccelerationStructureBuildInputReadOnly | Arc::BufferUsage::TransferDst,
		.MemoryProperty = Arc::MemoryProperty::DeviceLocal,
	});
	m_ResourceCache->CreateGpuBuffer(&model->IndexBuffer, Arc::GpuBufferDesc{
		.Size = (uint32_t)(indices.size() * sizeof(uint32_t)),
		.UsageFlags = Arc::BufferUsage::StorageBuffer | Arc::BufferUsage::ShaderDeviceAddress | Arc::BufferUsage::AccelerationStructureBuildInputReadOnly | Arc::BufferUsage::TransferDst,
		.MemoryProperty = Arc::MemoryProperty::DeviceLocal,
	});

	m_Device->SetDeviceLocalBufferData(&model->VertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex));
	m_Device->SetDeviceLocalBufferData(&model->IndexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

	m_ResourceCache->CreateBottomLevelAS(&model->BottomLevelAS, Arc::BottomLevelASDesc{
		.VertexBuffer = model->VertexBuffer.GetHandle(),
		.IndexBuffer = model->IndexBuffer.GetHandle(),
		.VertexStride = sizeof(Vertex),
		.NumTriangles = (uint32_t)indices.size() / 3,
		.VertexFormat = Arc::Format::R32G32B32_Sfloat
		});

	model->VertexBufferDeviceAddress = m_Device->GetBufferDeviceAddress(&model->VertexBuffer);
	model->IndexBufferDeviceAddress = m_Device->GetBufferDeviceAddress(&model->IndexBuffer);
}

void PathTracer::AddInstance(std::vector<MeshPrimitive>& meshInfos, std::vector<Material>& materials, Model* model, glm::mat4 transform, const Material& material)
{
	m_Scene->AddInstance(Arc::TopLevelASInstance{
		.BottomLevelASHandle = model->BottomLevelAS.GetHandle(),
		.InstanceCustomIndex = (uint32_t)meshInfos.size(),
		.TransformMatrix = {
			transform[0][0], transform[1][0], transform[2][0], transform[3][0],
			transform[0][1], transform[1][1], transform[2][1], transform[3][1],
			transform[0][2], transform[1][2], transform[2][2], transform[3][2],
		}
	});

	MeshPrimitive meshInfo{};
	meshInfo.VertexBufferDeviceAddress = model->VertexBufferDeviceAddress;
	meshInfo.IndexBufferDeviceAddress = model->IndexBufferDeviceAddress;

	uint32_t materialIndex = 0xFFFFFFFF;
	for (uint32_t i = 0; i < materials.size(); ++i)
	{
		if (materials[i] == material)
		{
			materialIndex = i;
			break;
		}
	}

	if (materialIndex == 0xFFFFFFFF)
	{
		materialIndex = static_cast<uint32_t>(materials.size());
		materials.push_back(material);
	}
	meshInfo.MaterialIndex = materialIndex;
	meshInfos.push_back(meshInfo);
}

void PathTracer::CreateAccelerationStructure()
{
	m_Plane = std::make_unique<Model>();
	LoadModel("res/3DModels/plane.obj", m_Plane.get());
	m_Dragon = std::make_unique<Model>();
	LoadModel("res/3DModels/dragon.obj", m_Dragon.get());
	m_Sphere = std::make_unique<Model>();
	LoadModel("res/3DModels/sphere.obj", m_Sphere.get());
	m_Scene = std::make_unique<Arc::TopLevelAS>(); 
	m_ResourceCache->CreateTopLevelAS(m_Scene.get(), Arc::TopLevelASDesc{});

	std::vector<MeshPrimitive> meshInfos;
	std::vector<Material> materials;
	Material glass{};
	glass.Color = glm::vec4(0.95, 0.7, 0.7, 1);
	glass.Roughness = 0.1f;
	glass.Transmission = 1.0f;
	Material redWall{};
	redWall.Color = vec4(1.0, 0.2, 0.3, 1);
	redWall.TextureIndex = 1;
	Material greenWall{};
	greenWall.Color = vec4(0.25, 1.0, 0.35, 1);
	greenWall.Metallic = 1.0f;
	Material blueWall{};
	blueWall.Color = vec4(0.25, 0.55, 1.0, 1);
	Material whiteWall{};
	whiteWall.Color = vec4(0.8, 0.8, 0.8, 1);
	Material light{};
	light.Color = vec4(1);
	light.Emission = vec4(15.0, 15.0, 15.0, 1);

	{ // FLOOR
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 0.0f, 0.0f));
		mat4 R = mat4(1.0f);
		mat4 S = scale(mat4(1.0f), vec3(1.0f, 1.0f, 1.0f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, whiteWall);
	}
	{ // CEILING
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 1.0f, 0.0f));
		mat4 R = rotate(mat4(1.0f), radians(180.0f), vec3(1, 0, 0));
		mat4 S = scale(mat4(1.0f), vec3(1.0f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, whiteWall);
	}
	{ // BACK
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 0.5f, 0.5f));
		mat4 R = rotate(mat4(1.0f), radians(-90.0f), vec3(1, 0, 0));
		mat4 S = scale(mat4(1.0f), vec3(1.0f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, blueWall);
	}
	{ // LEFT
		mat4 T = translate(mat4(1.0f), vec3(-0.5f, 0.5f, 0.0f));
		mat4 R = rotate(mat4(1.0f), radians(-90.0f), vec3(0, 0, 1));
		mat4 S = scale(mat4(1.0f), vec3(1.0f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, redWall);
	}
	{ // RIGHT
		mat4 T = translate(mat4(1.0f), vec3(+0.5f, 0.5f, 0.0f));
		mat4 R = rotate(mat4(1.0f), radians(90.0f), vec3(0, 0, 1));
		mat4 S = scale(mat4(1.0f), vec3(1.0f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, greenWall);
	}
	{ // LIGHT
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 0.999f, 0.0f));
		mat4 R = rotate(mat4(1.0f), radians(180.0f), vec3(1, 0, 0));
		mat4 S = scale(mat4(1.0f), vec3(0.25f));
		AddInstance(meshInfos, materials, m_Plane.get(), T * R * S, light);
	}
	{ // DRAGON
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 0.26f, 0.0f));
		mat4 R = rotate(mat4(1.0f), radians(70.0f), vec3(0, 1, 0));
		mat4 S = scale(mat4(1.0f), vec3(0.9f));
		AddInstance(meshInfos, materials, m_Dragon.get(), T * R * S, glass);
	}
	{ // SPHERE
		mat4 T = translate(mat4(1.0f), vec3(0.0f, 0.7f, 0.0f));
		mat4 R = mat4(1.0f);
		mat4 S = scale(mat4(1.0f), vec3(0.3f));
		//AddInstance(meshInfos, materials, m_Sphere.get(), T * R * S, glass);
	}
	m_Scene->Build();
	
	m_MeshInfoBuffer = std::make_unique<Arc::GpuBuffer>();
	m_ResourceCache->CreateGpuBuffer(m_MeshInfoBuffer.get(), Arc::GpuBufferDesc{
		.Size = (uint32_t)meshInfos.size() * (uint32_t)sizeof(MeshPrimitive),
		.UsageFlags = Arc::BufferUsage::StorageBuffer | Arc::BufferUsage::ShaderDeviceAddress,
		.MemoryProperty = Arc::MemoryProperty::HostVisible
	});

	void* data = m_ResourceCache->MapMemory(m_MeshInfoBuffer.get());
	memcpy(data, meshInfos.data(), meshInfos.size() * sizeof(MeshPrimitive));
	m_ResourceCache->UnmapMemory(m_MeshInfoBuffer.get());

	m_MaterialBuffer = std::make_unique<Arc::GpuBuffer>();
	m_ResourceCache->CreateGpuBuffer(m_MaterialBuffer.get(), Arc::GpuBufferDesc{
		.Size = (uint32_t)materials.size() * (uint32_t)sizeof(Material),
		.UsageFlags = Arc::BufferUsage::StorageBuffer | Arc::BufferUsage::ShaderDeviceAddress,
		.MemoryProperty = Arc::MemoryProperty::HostVisible
	});

	data = m_ResourceCache->MapMemory(m_MaterialBuffer.get());
	memcpy(data, materials.data(), materials.size() * sizeof(Material));
	m_ResourceCache->UnmapMemory(m_MaterialBuffer.get());


	m_WhiteTexture = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_WhiteTexture.get(), Arc::GpuImageDesc{
		.Extent = { 1, 1, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferDst | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	uint32_t whitePixel = 0xFFFFFFFF;
	m_Device->SetImageData(m_WhiteTexture.get(), &whitePixel, sizeof(uint32_t), Arc::ImageLayout::ShaderReadOnlyOptimal);

	int imgWidth, imgHeight, imgChannels;
	const char* file = "res/Images/Granite.jpg";
	stbi_info(file, &imgWidth, &imgHeight, &imgChannels);
	m_Texture = std::make_unique<Arc::GpuImage>();
	m_ResourceCache->CreateGpuImage(m_Texture.get(), Arc::GpuImageDesc{
		.Extent = { (uint32_t)imgWidth, (uint32_t)imgHeight, 1},
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::TransferDst | Arc::ImageUsage::Sampled,
		.AspectFlags = Arc::ImageAspect::Color,
		});
	data = stbi_load(file, &imgWidth, &imgHeight, &imgChannels, 4);
	m_Device->SetImageData(m_Texture.get(), data, imgWidth * imgHeight * 4 * sizeof(uint8_t), Arc::ImageLayout::ShaderReadOnlyOptimal);
	stbi_image_free(data);


	m_SceneDescriptorSet = std::make_unique<Arc::DescriptorSet>();
	m_ResourceCache->AllocateDescriptorSet(m_SceneDescriptorSet.get(), Arc::DescriptorSetDesc{
		.Bindings = {
			{ Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::RayClosestHit },
			{ Arc::DescriptorType::StorageBuffer, Arc::ShaderStage::RayClosestHit },
			{ Arc::DescriptorType::CombinedImageSampler, Arc::ShaderStage::RayClosestHit, Arc::DescriptorFlag::Bindless },
		}
	});
	m_Device->UpdateDescriptorSet(m_SceneDescriptorSet.get(), Arc::DescriptorWrite()
		.AddWrite(Arc::BufferWrite(0, m_MeshInfoBuffer.get()))
		.AddWrite(Arc::BufferWrite(1, m_MaterialBuffer.get()))
		.AddWrite(Arc::ImageWrite(2, 0, m_WhiteTexture.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
		.AddWrite(Arc::ImageWrite(2, 1, m_Texture.get(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_LinearSampler.get()))
	);
}

void PathTracer::CreatePipelines()
{
	m_RayGenShader = std::make_unique<Arc::Shader>();
	m_RayMissShader = std::make_unique<Arc::Shader>();
	m_RayClosestHitShader = std::make_unique<Arc::Shader>();
	m_CompositeVertShader = std::make_unique<Arc::Shader>();
	m_CompositeFragShader = std::make_unique<Arc::Shader>();

	Arc::ShaderDesc shaderDesc;
	if (Arc::ShaderCompiler::Compile("res/Shaders/PathTracer/raygen.rgen", shaderDesc))
		m_ResourceCache->CreateShader(m_RayGenShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/PathTracer/raymiss.rmiss", shaderDesc))
		m_ResourceCache->CreateShader(m_RayMissShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/PathTracer/rayclosesthit.rchit", shaderDesc))
		m_ResourceCache->CreateShader(m_RayClosestHitShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/PathTracer/shader.vert", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeVertShader.get(), shaderDesc);
	if (Arc::ShaderCompiler::Compile("res/Shaders/PathTracer/shader.frag", shaderDesc))
		m_ResourceCache->CreateShader(m_CompositeFragShader.get(), shaderDesc);

	m_RayTracingPipeline = std::make_unique<Arc::RayTracingPipeline>();
	m_ResourceCache->CreateRayTracingPipeline(m_RayTracingPipeline.get(), Arc::RayTracingPipelineDesc{
		.ShaderStages = { m_RayGenShader.get(), m_RayMissShader.get(), m_RayClosestHitShader.get()},
		.PushDescriptorSets = { 1 }
	});

	m_CompositePipeline = std::make_unique<Arc::Pipeline>();;
	m_ResourceCache->CreatePipeline(m_CompositePipeline.get(), Arc::PipelineDesc{
		.ShaderStages = { m_CompositeVertShader.get(), m_CompositeFragShader.get() },
		.Topology = Arc::PrimitiveTopology::TriangleFan,
		.ColorAttachmentFormats = { m_PresentQueue->GetSurfaceFormat() },
		.UsePushDescriptors = true
	});
}

void PathTracer::CreateSamplers()
{
	m_NearestSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_NearestSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Nearest,
		.MagFilter = Arc::Filter::Nearest,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});
	m_LinearSampler = std::make_unique<Arc::Sampler>();
	m_ResourceCache->CreateSampler(m_LinearSampler.get(), Arc::SamplerDesc{
		.MinFilter = Arc::Filter::Linear,
		.MagFilter = Arc::Filter::Linear,
		.AddressMode = Arc::SamplerAddressMode::Repeat
		});
}

void PathTracer::CreateImages()
{
	uint32_t w = m_PresentQueue->GetExtent()[0];
	uint32_t h = m_PresentQueue->GetExtent()[1];

	m_Device->WaitIdle();

	if (m_OutputImage.get())
		m_ResourceCache->ReleaseResource(m_OutputImage.get());

	m_OutputImage = std::make_unique<Arc::GpuImage>();
	auto imageDesc = Arc::GpuImageDesc{
		.Extent = { w, h, 1 },
		.Format = Arc::Format::R8G8B8A8_Unorm,
		.UsageFlags = Arc::ImageUsage::Storage | Arc::ImageUsage::Sampled | Arc::ImageUsage::TransferSrc,
		.AspectFlags = Arc::ImageAspect::Color,
	};
	m_ResourceCache->CreateGpuImage(m_OutputImage.get(), imageDesc);

	if (m_AccumulationImage1.get())
		m_ResourceCache->ReleaseResource(m_AccumulationImage1.get());
	if (m_AccumulationImage2.get())
		m_ResourceCache->ReleaseResource(m_AccumulationImage2.get());
	m_AccumulationImage1 = std::make_unique<Arc::GpuImage>();
	m_AccumulationImage2 = std::make_unique<Arc::GpuImage>();
	{
		Arc::GpuImageDesc desc = Arc::GpuImageDesc{
		.Extent = { w, h, 1},
		.Format = Arc::Format::R32G32B32A32_Sfloat,
		.UsageFlags = Arc::ImageUsage::TransferDst | Arc::ImageUsage::Storage,
		.AspectFlags = Arc::ImageAspect::Color,
		.MipLevels = 1,
		};
		m_ResourceCache->CreateGpuImage(m_AccumulationImage1.get(), desc);
		m_ResourceCache->CreateGpuImage(m_AccumulationImage2.get(), desc);
		const float clearColor[4] = {0.0, 0.0, 0.0, 0.0};
		m_Device->ImmediateSubmit([&](Arc::CommandBufferHandle cmdHandle) {
			Arc::CommandBuffer cmd(cmdHandle);
			cmd.TransitionImage(m_AccumulationImage1->GetHandle(), Arc::ImageLayout::Undefined, Arc::ImageLayout::General);
			cmd.TransitionImage(m_AccumulationImage2->GetHandle(), Arc::ImageLayout::Undefined, Arc::ImageLayout::General);
			cmd.ClearColorImage(m_AccumulationImage1->GetHandle(), clearColor, Arc::ImageLayout::General);
			cmd.ClearColorImage(m_AccumulationImage2->GetHandle(), clearColor, Arc::ImageLayout::General);
		});
	}
	m_Device->WaitIdle();

	m_Camera->AspectRatio = w / (float)h;
}

PathTracer::~PathTracer()
{

}

void PathTracer::RenderFrame(float elapsedTime)
{
	static float lastTime = 0.0f;
	float dt = elapsedTime - lastTime;
	lastTime = elapsedTime;
	m_IsEvenFrame = !m_IsEvenFrame;

	if (Arc::Input::IsKeyPressed(Arc::KeyCode::G))
	{
		std::vector<uint8_t> imageData = m_Device->GetImageData(m_OutputImage.get(), Arc::ImageLayout::ShaderReadOnlyOptimal);
		std::string path = "img.png";
		stbi_write_png(path.c_str(), m_OutputImage->GetExtent()[0], m_OutputImage->GetExtent()[1], 4, imageData.data(), m_OutputImage->GetExtent()[0] * 4);
		ARC_LOG("Screenshot saved to disk");
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(16));

	Arc::FrameData frameData = m_PresentQueue->BeginFrame();
	Arc::CommandBuffer* cmd = frameData.CommandBuffer;

	m_Camera->Update(dt);

	globalFrameData.CameraPosition = glm::vec4(m_Camera->Position, 0);
	globalFrameData.CameraForward = glm::vec4(m_Camera->Forward, 0);
	globalFrameData.CameraRight = glm::vec4(m_Camera->Right, 0);
	globalFrameData.CameraUp = glm::vec4(-m_Camera->Up, 0);
	globalFrameData.TanHalfFov = glm::tan(m_Camera->Fov * 0.5);
	globalFrameData.AspectRatio = m_Camera->AspectRatio;
	globalFrameData.Aperture = 0.0;
	globalFrameData.FocusDistance = 1.0;
	globalFrameData.FrameIndex++;

	if (m_Camera->HasMoved || Arc::Input::IsKeyDown(Arc::KeyCode::C))
	{
		globalFrameData.FrameIndex = 1;
	}

	{
		void* data = m_ResourceCache->MapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
		memcpy(data, &globalFrameData, sizeof(GlobalFrameData));
		m_ResourceCache->UnmapMemory(m_GlobalDataBuffer.get(), frameData.FrameIndex);
	}

	m_RenderGraph->AddPass(Arc::RenderPass{
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->MemoryBarrier({
			Arc::CommandBuffer::ImageBarrier{
				.Handle = m_OutputImage->GetHandle(),
				.OldLayout = Arc::ImageLayout::Undefined,
				.NewLayout = Arc::ImageLayout::General,
			} });

			cmd->BindDescriptorSets(Arc::PipelineBindPoint::RayTracing, m_RayTracingPipeline->GetLayout(), 0, { m_SceneDescriptorSet->GetHandle() });
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::RayTracing, m_RayTracingPipeline->GetLayout(), 1, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushAccelerationStructureWrite(0, Arc::DescriptorType::AccelerationStructure, m_Scene->GetHandle()))
				.AddWrite(Arc::PushImageWrite(1, Arc::DescriptorType::StorageImage, m_IsEvenFrame ? m_AccumulationImage1->GetImageView() : m_AccumulationImage2->GetImageView(), Arc::ImageLayout::General, nullptr))
				.AddWrite(Arc::PushImageWrite(2, Arc::DescriptorType::StorageImage, m_IsEvenFrame ? m_AccumulationImage2->GetImageView() : m_AccumulationImage1->GetImageView(), Arc::ImageLayout::General, nullptr))
				.AddWrite(Arc::PushImageWrite(3, Arc::DescriptorType::StorageImage, m_OutputImage->GetImageView(), Arc::ImageLayout::General, nullptr))
				.AddWrite(Arc::PushBufferWrite(4, Arc::DescriptorType::UniformBuffer, m_GlobalDataBuffer->GetHandle(frameIndex), m_GlobalDataBuffer->GetSize())));
			cmd->BindRayTracingPipeline(m_RayTracingPipeline->GetHandle());
			cmd->TraceRays(m_RayTracingPipeline.get(), m_OutputImage->GetExtent()[0], m_OutputImage->GetExtent()[1], 1);
		
			cmd->MemoryBarrier({
			Arc::CommandBuffer::ImageBarrier{
				.Handle = m_OutputImage->GetHandle(),
				.OldLayout = Arc::ImageLayout::General,
				.NewLayout = Arc::ImageLayout::ShaderReadOnlyOptimal
			} });
		}
	});

	m_RenderGraph->SetPresentPass(Arc::PresentPass{
		.LoadOp = Arc::AttachmentLoadOp::Clear,
		.ClearColor = {0.1, 0.6, 0.6, 1.0},
		.ExecuteFunction = [&](Arc::CommandBuffer* cmd, uint32_t frameIndex) {
			cmd->BindPipeline(m_CompositePipeline->GetHandle());
			cmd->PushDescriptorSets(Arc::PipelineBindPoint::Graphics, m_CompositePipeline->GetLayout(), 0, Arc::PushDescriptorWrite()
				.AddWrite(Arc::PushImageWrite(0, Arc::DescriptorType::CombinedImageSampler, m_OutputImage->GetImageView(), Arc::ImageLayout::ShaderReadOnlyOptimal, m_NearestSampler->GetHandle())));
			cmd->Draw(6, 1, 0, 0);
		}
	});

	m_RenderGraph->BuildGraph();
	m_RenderGraph->Execute(frameData, m_PresentQueue->GetExtent());
	m_PresentQueue->EndFrame();
}

void PathTracer::SwapchainResized(void* presentQueue)
{
	m_PresentQueue = static_cast<Arc::PresentQueue*>(presentQueue);
	globalFrameData.FrameIndex = 0;
	CreateImages();
}

void PathTracer::RecompileShaders()
{

}

void PathTracer::WaitForFrameEnd()
{

}