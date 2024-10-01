#pragma once
#include "ArcaneEngine/Graphics/VulkanObjects/Shader.h"
#include <string>

namespace Arc::ShaderCompiler
{
	bool Compile(const std::string& filePath, ShaderDesc& shaderDesc);
	bool CompileFromSource(const std::string& source, ShaderStage shaderStage, ShaderDesc& shaderDesc, const std::string& debugName);
}