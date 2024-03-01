#include "Shader.h"

namespace Arc
{
	ShaderDesc& ShaderDesc::SetSpirv(const std::vector<uint32_t>& spirv)
	{
		this->SpirV = spirv;
		return *this;
	}

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