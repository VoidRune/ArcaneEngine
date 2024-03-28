#pragma once
#include "Renderer/VertexDefinitions.h"
#include <string>
#include <unordered_map>

class TextBuilder
{
public:
	TextBuilder() {};

	void LoadFont(std::string filePath);

	struct TextDescription
	{
		glm::vec2 ScreenPosition;
		glm::vec4 TopColor;
		glm::vec4 BottomColor;
		int32_t PixelHeight;
		int32_t AllowedWidth = 4096;
		bool Centered = false;
	};

	void BuildText(const std::string& text, const TextDescription& desc, std::vector<GuiVertex>& vertices);
	bool IsLoaded() { return m_Loaded; }

private:
	struct Character
	{
		int32_t width;
		int32_t height;
		int32_t xoffset;
		int32_t yoffset;
		int32_t xadvance;
		int32_t x;
		int32_t y;
	};

	bool m_Loaded = false;
	int32_t m_FontSize = 0;
	int32_t m_LineHeight = 0;
	int32_t m_TextureWidth = 0;
	int32_t m_TextureHeight = 0;
	std::unordered_map<char, Character> m_Characters;
};