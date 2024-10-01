#pragma once
#include "ArcaneEngine/Graphics/Common.h"
#include <vector>
#include <cstdint>

namespace Arc
{
	class VertexAttributes
	{
	public:
		struct Attribute
		{
			Attribute(Format format, uint32_t offset) : format(format), offset(offset) {}
			Format format;
			uint32_t offset;
		};

		VertexAttributes() : m_Attributes({ }), m_VertexSize(0) {}
		VertexAttributes(const std::vector<Attribute>& attributes, uint32_t vertexSize) : m_Attributes(attributes), m_VertexSize(vertexSize) {}
	private:
		std::vector<Attribute> m_Attributes;
		uint32_t m_VertexSize;

		friend class ResourceCache;
	};
}