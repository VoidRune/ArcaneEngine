#pragma once
#include "ArcaneEngine/Graphics/VulkanCore/VulkanHandles.h"
#include "ArcaneEngine/Graphics/Common.h"

namespace Arc
{
	struct SamplerDesc
	{
		Filter MinFilter = Filter::Linear;
		Filter MagFilter = Filter::Linear;
		SamplerAddressMode AddressMode = SamplerAddressMode::MirroredRepeat;
	};

	class Sampler
	{
	public:
		SamplerHandle GetHandle() { return m_Sampler; }
	private:
		SamplerHandle m_Sampler;

		friend class ResourceCache;
	};
}