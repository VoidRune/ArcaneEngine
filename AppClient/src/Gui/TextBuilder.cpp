#include "TextBuilder.h"
#include "tinygltf/json.hpp"
#include <fstream>

using json = nlohmann::json;

void TextBuilder::LoadFont(std::string filePath)
{
	std::ifstream f(filePath);
	json data = json::parse(f);

    m_FontSize = data["info"]["size"];
    m_LineHeight = data["common"]["lineHeight"];
    m_TextureWidth = data["common"]["scaleW"];
    m_TextureHeight = data["common"]["scaleH"];
    auto chars = data["chars"];
	for (auto& ch : chars)
	{
		std::string id = ch["char"];
		Character c;
		c.width = ch["width"];
		c.height = ch["height"];
		c.xoffset = ch["xoffset"];
		c.yoffset = ch["yoffset"];
		c.xadvance = ch["xadvance"];
		c.x = ch["x"];
		c.y = ch["y"];
		m_Characters[id[0]] = c;
	}

    m_Loaded = true;
}

void TextBuilder::BuildText(const std::string& text, const TextDescription& desc, std::vector<GuiVertex>& vertices)
{
    float x = desc.ScreenPosition.x;
    float y = desc.ScreenPosition.y;

    float scaleRatio = desc.PixelHeight / (float)m_FontSize;

	for (char id : text)
	{
		Character c = m_Characters[id];
        //c.width *= scaleRatio;
        //c.height *= scaleRatio;
        //c.x *= scaleRatio;
        //c.y *= scaleRatio;
        c.xadvance *= scaleRatio;
        c.xoffset *= scaleRatio;
        c.yoffset *= scaleRatio;

        float charWidth = c.width * scaleRatio;
        float charHeight = c.height * scaleRatio;

        if (x + charWidth + c.xoffset > desc.AllowedWidth)
        {
            x = desc.ScreenPosition.x;
            y += m_LineHeight * scaleRatio;
        }

        if (id == ' ')
        {
            x += c.xadvance;
            continue;
        }
        else if (id == '\n')
        {
            x = desc.ScreenPosition.x;
            y += m_LineHeight * scaleRatio;
            continue;
        }

        glm::vec4 texCoord = { 
            c.x / (float)m_TextureWidth, 1.0f - c.y / (float)m_TextureHeight,
            (c.x + c.width) / (float)m_TextureWidth, 1.0f - (c.y + c.height) / (float)m_TextureHeight };
      
        vertices.push_back({ { c.xoffset + x, c.yoffset + charHeight + y }, {texCoord.x, texCoord.w}, desc.BottomColor, 0 });
        vertices.push_back({ { c.xoffset + x, c.yoffset + y }, {texCoord.x, texCoord.y}, desc.TopColor, 0 });
        vertices.push_back({ { c.xoffset + x + charWidth, c.yoffset + charHeight + y }, {texCoord.z, texCoord.w}, desc.BottomColor, 0 });

        vertices.push_back({ { c.xoffset + x + charWidth, c.yoffset + charHeight + y }, {texCoord.z, texCoord.w}, desc.BottomColor, 0 });
        vertices.push_back({ { c.xoffset + x, c.yoffset + y }, {texCoord.x, texCoord.y}, desc.TopColor, 0 });
        vertices.push_back({ { c.xoffset + x + charWidth, c.yoffset + y }, {texCoord.z, texCoord.y}, desc.TopColor, 0 });

        x += c.xadvance;
	}

}