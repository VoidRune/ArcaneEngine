#include "Shader.h"
#include "SpirvCompiler.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <mutex>

namespace Arc
{
	ShaderDesc& ShaderDesc::SetFilePath(const std::string& filePath)
	{
		this->FilePath = filePath;
		return *this;
	}

	ShaderDesc& ShaderDesc::SetEntryPoint(const std::string& entryPoint)
	{
		this->EntryPoint = entryPoint;
		return *this;
	}
}