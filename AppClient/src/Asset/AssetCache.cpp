#include "AssetCache.h"
#include "Renderer/Renderer.h"
#include "stb/stb_image.h"
#include "tinyobj/tiny_obj_loader.h"
#include "Core/Log.h"

AssetCache::AssetCache(Arc::Device* device)
{
	m_Device = device;
}

AssetCache::~AssetCache()
{

}

void AssetCache::LoadObj(Mesh* mesh, std::string filePath)
{
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = ""; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(filePath, reader_config)) {
		std::cout << "Failed to parse obj file!" << std::endl;
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	std::vector<StaticVertex> vertices;
	std::vector<uint32_t> indices;

	vertices.reserve(shapes[0].mesh.num_face_vertices.size());

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {

				StaticVertex vertex;
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				vertex.Position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				vertex.Position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				vertex.Position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
				vertex.Normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
				vertex.Normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
				vertex.Normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
				vertex.TexCoord.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
				vertex.TexCoord.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

				vertices.push_back(vertex);
				indices.push_back(indices.size());
			}

			StaticVertex& v1 = vertices[vertices.size() - 1];
			StaticVertex& v2 = vertices[vertices.size() - 2];
			StaticVertex& v3 = vertices[vertices.size() - 3];

			glm::vec3 edge1 = v2.Position - v1.Position;
			glm::vec3 edge2 = v3.Position - v1.Position;
			glm::vec2 deltaUV1 = v2.TexCoord - v1.TexCoord;
			glm::vec2 deltaUV2 = v3.TexCoord - v1.TexCoord;

			float mu = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = mu * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = mu * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = mu * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			v1.Tangent = tangent;
			v2.Tangent = tangent;
			v3.Tangent = tangent;

			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	mesh->VertexCount = vertices.size();
	mesh->IndexCount = indices.size();

	m_Device->GetResourceCache()->CreateBuffer(&mesh->VertexBuffer, Arc::GpuBufferDesc()
		.SetSize(vertices.size() * sizeof(StaticVertex))
		.AddBufferUsage(Arc::BufferUsage::VertexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
	m_Device->UploadToDeviceLocalBuffer(&mesh->VertexBuffer, vertices.data(), vertices.size() * sizeof(StaticVertex));

	m_Device->GetResourceCache()->CreateBuffer(&mesh->IndexBuffer, Arc::GpuBufferDesc()
		.SetSize(indices.size() * sizeof(uint32_t))
		.AddBufferUsage(Arc::BufferUsage::IndexBuffer).AddBufferUsage(Arc::BufferUsage::TransferDst)
		.AddMemoryPropertyFlag(Arc::MemoryProperty::DeviceLocal));
	m_Device->UploadToDeviceLocalBuffer(&mesh->IndexBuffer, indices.data(), indices.size() * sizeof(uint32_t));

}


void AssetCache::LoadImage(Texture* texture, std::string filePath)
{
	stbi_set_flip_vertically_on_load(true);
	int w, h, c;

	unsigned char* data = stbi_load(filePath.c_str(), &w, &h, &c, 4);
	Arc::Image image;
	m_Device->GetResourceCache()->CreateImage(&image, Arc::ImageDesc()
		.SetExtent({ (uint32_t)w, (uint32_t)h })
		.SetFormat(Arc::Format::R8G8B8A8_Unorm)
		.AddUsageFlag(Arc::ImageUsage::TransferDst)
		.AddUsageFlag(Arc::ImageUsage::Sampled)
		.SetEnableMipLevels(true));
	m_Device->SetImageData(&image, data, w * h * 4, Arc::ImageLayout::ShaderReadOnlyOptimal);
	stbi_image_free(data);

	texture->TextureBinding = Renderer::BindTexture(image);
}