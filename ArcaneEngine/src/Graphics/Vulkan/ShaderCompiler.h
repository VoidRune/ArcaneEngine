#pragma once
#include "Shader.h"
#include <string>

namespace Arc::ShaderCompiler
{
	void Initialize();
	void Finalize();

	bool Compile(const std::string& filePath, ShaderDesc& shaderDesc);
}