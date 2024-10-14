#include "ShaderCompiler.h"
#include "ArcaneEngine/Core/Log.h"
#include <fstream>
#include <filesystem>

//#include <glslang/Include/glslang_c_interface.h>
//#include <glslang/Public/resource_limits_c.h>

#include <shaderc/shaderc.hpp>

namespace Arc::ShaderCompiler
{
	bool ReadFile(const std::string& filePath, std::string& dataText)
	{
		std::ifstream in(filePath, std::ios::in | std::ios::binary);
		if (in.is_open())
		{
			in.seekg(0, std::ios::end);
			dataText.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(dataText.data(), dataText.size());
			in.close();
		}
		else
		{
			return false;
		}
		return true;
	}

	class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
	public:
		shaderc_include_result* GetInclude(const char* requested_source,
			shaderc_include_type type,
			const char* requesting_source,
			size_t include_depth) override {

			std::filesystem::path fullPath = requesting_source;
			std::string parentPath = fullPath.parent_path().string();
			std::string path = parentPath + "/" + requested_source;
			std::string dataText;
			if (!ReadFile(parentPath + "/" + requested_source, dataText))
			{
				ARC_LOG_ERROR(std::string("Failed to compile shader: Could not open file ") + path);
				return nullptr;
			}

			auto* result = new shaderc_include_result;
			result->source_name = requested_source;
			result->source_name_length = strlen(requested_source);
			char* content_ptr = new char[dataText.size() + 1];
			strcpy(content_ptr, dataText.c_str());
			result->content = content_ptr;
			result->content_length = dataText.size();
			result->user_data = nullptr;

			return result;
		}

		void ReleaseInclude(shaderc_include_result* data) override {
			delete[] data->content;
			delete data;
		}
	};

	bool Compile(const std::string& filePath, ShaderDesc& shaderDesc)
	{
		std::string dataText;
		if (!ReadFile(filePath, dataText))
		{
			ARC_LOG_ERROR(std::string("Failed to compile shader: Could not open file ") + filePath.c_str());
			return false;
		}

		std::filesystem::path path = filePath;
		std::string extension = path.extension().string();

		auto shaderStage = ShaderStage::Vertex;
		if (extension == ".vert") shaderStage = ShaderStage::Vertex;
		else if (extension == ".frag") shaderStage = ShaderStage::Fragment;
		else if (extension == ".comp") shaderStage = ShaderStage::Compute;
		else
		{
			ARC_LOG_ERROR(std::string("Failed to descipher shader stage from file: ") + filePath.c_str());
		}

		return CompileFromSource(dataText, shaderStage, shaderDesc, filePath);
	}

	bool CompileFromSource(const std::string& source, ShaderStage shaderStage, ShaderDesc& shaderDesc, const std::string& debugName)
	{	
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetIncluder(std::make_unique<ShaderIncluder>());

		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
		const bool optimize = true;
		if (optimize)
			options.SetOptimizationLevel(shaderc_optimization_level_performance);

		auto shaderStageShaderc = shaderc_glsl_vertex_shader;

		switch (shaderStage)
		{
		case Arc::ShaderStage::Vertex: shaderStageShaderc = shaderc_glsl_vertex_shader; break;
		case Arc::ShaderStage::Fragment: shaderStageShaderc = shaderc_glsl_fragment_shader; break;
		case Arc::ShaderStage::Compute: shaderStageShaderc = shaderc_glsl_compute_shader; break;
		default:
		{
			ARC_LOG_ERROR(std::string("Unknown shader stage: ") + debugName);
		}
		}

		auto precompileResult = compiler.PreprocessGlsl(source.data(), shaderStageShaderc, debugName.c_str(), options);
		if (precompileResult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			//LOG_CORE_ERROR("VK_Shader: Could not preompile shader {0}, error message: {1}", m_SourceFilepath, precompileResult.GetErrorMessage());
			ARC_LOG_ERROR(std::string("Unknown shader stage: ") + debugName);
		}

		shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source.data(), shaderStageShaderc, debugName.c_str(), options);
		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			ARC_LOG_ERROR(std::string("Failed to compile shader: ") + module.GetErrorMessage());
			return false;
		}

		shaderDesc.SpirV = std::vector<uint32_t>(module.cbegin(), module.cend());
		shaderDesc.ShaderStage = shaderStage;

		return true;

		/*
		auto shaderStageGlslang = GLSLANG_STAGE_VERTEX;

		switch (shaderStage)
		{
		case Arc::ShaderStage::Vertex: shaderStageGlslang = GLSLANG_STAGE_VERTEX; break;
		case Arc::ShaderStage::Fragment: shaderStageGlslang = GLSLANG_STAGE_FRAGMENT; break;
		case Arc::ShaderStage::Compute: shaderStageGlslang = GLSLANG_STAGE_COMPUTE; break;
		default:
		{
			ARC_LOG_ERROR(std::string("Unknown shader stage: ") + debugName);
		}
		}

		if (!glslang_initialize_process())
		{
			ARC_LOG_ERROR(std::string("Failed to initialize glslang process!"));
		}

		static const glslang_resource_t* defaultResources = glslang_default_resource();
		const glslang_input_t input = {
			.language = GLSLANG_SOURCE_GLSL,
			.stage = shaderStageGlslang,
			.client = GLSLANG_CLIENT_VULKAN,
			.client_version = GLSLANG_TARGET_VULKAN_1_3,
			.target_language = GLSLANG_TARGET_SPV,
			.target_language_version = GLSLANG_TARGET_SPV_1_5,
			.code = source.c_str(),
			.default_version = 100,
			.default_profile = GLSLANG_NO_PROFILE,
			.force_default_version_and_profile = false,
			.forward_compatible = false,
			.messages = GLSLANG_MSG_DEFAULT_BIT,
			.resource = defaultResources,
		};

		glslang_shader_t* shader = glslang_shader_create(&input);

		if (!glslang_shader_preprocess(shader, &input))
		{
			ARC_LOG_ERROR(std::string("GLSL preprocessing failed: ") + debugName + "  " +
						  std::string(glslang_shader_get_info_log(shader)) + "  " +
						  std::string(glslang_shader_get_info_debug_log(shader)));
			glslang_shader_delete(shader);
			return false;
		}

		if (!glslang_shader_parse(shader, &input))
		{
			ARC_LOG_ERROR(std::string("GLSL parsing failed: ") + debugName + "  " +
				std::string(glslang_shader_get_info_log(shader)) + "  " +
				std::string(glslang_shader_get_info_debug_log(shader)));
			glslang_shader_delete(shader);
			return false;
		}

		glslang_program_t* program = glslang_program_create();
		glslang_program_add_shader(program, shader);

		if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
			ARC_LOG_ERROR(std::string("GLSL linking failed: ") + debugName + "  " +
				std::string(glslang_shader_get_info_log(shader)) + "  " +
				std::string(glslang_shader_get_info_debug_log(shader)));
			glslang_program_delete(program);
			glslang_shader_delete(shader);
		}

		glslang_program_SPIRV_generate(program, shaderStageGlslang);

		std::vector<uint32_t> spirv(glslang_program_SPIRV_get_size(program));
		glslang_program_SPIRV_get(program, spirv.data());

		const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
		if (spirv_messages)
		{
			ARC_LOG_ERROR(std::string(spirv_messages) + ": " + debugName);
		}

		glslang_program_delete(program);
		glslang_shader_delete(shader);

		glslang_finalize_process();

		shaderDesc.SpirV = spirv;
		shaderDesc.ShaderStage = shaderStage;

		return true;
		*/
	}
}