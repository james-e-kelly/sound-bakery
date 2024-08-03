#include "image.h"

#include "gluten/theme/theme.h"

#include "imgui.h"
#include "imgui_internal.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

namespace gluten 
{
	image::image(const cmrc::embedded_filesystem& filesystem, const std::string& filePath)
    {
        const cmrc::file file = filesystem.open(filePath);
        
		if (image::data_ptr imageData = get_image_data((stbi_uc*)file.begin(), file.size()))
		{
            bind_image_data(imageData);
		}
	}

    image::image(const void* data, std::size_t dataSize) 
    {
        if (image::data_ptr imageData = get_image_data((unsigned char*)data, dataSize))
        {
            bind_image_data(imageData);
        }
    }

    image::~image() 
    {
        release();
    }

    bool image::render_element(const ImRect& elementRect)
    {
        if (m_openGlId != 0 && m_width > 0 && m_height > 0)
        {
            if (ImDrawList* const drawList = ImGui::GetForegroundDrawList())
            {
                const ImVec2 elementRectSize = elementRect.GetSize();

                const float imageWidthRatio  = std::clamp(elementRectSize.x, 0.0f, m_maxSize.x > 0.0f ? m_maxSize.x : elementRectSize.x) / m_width;
                const float imageHeightRatio = std::clamp(elementRectSize.y, 0.0f, m_maxSize.y > 0.0f ? m_maxSize.y : elementRectSize.y) / m_height;
                const float lengthWithLeastAmountOfSpace = std::min(imageWidthRatio, imageHeightRatio);

                const float newImageWidth = m_width * lengthWithLeastAmountOfSpace;
                const float newImageHeight = m_height * lengthWithLeastAmountOfSpace;

                const float widthMovementAfterResize  = newImageWidth - m_width;
                const float heightMovementAfterResize = newImageHeight - m_height;

                const float newStartX = elementRect.Min.x + (elementRectSize.x / 2) - (newImageWidth / 2);
                const float newStartY = elementRect.Min.y + (elementRectSize.y / 2) - (newImageHeight / 2);

                drawList->AddImage((void*)(intptr_t)m_openGlId, 
                    ImVec2(newStartX, newStartY), 
                    ImVec2(newStartX + newImageWidth, newStartY + newImageHeight));
               
                //ImGui::DebugDrawItemRect(gluten::theme::invalidPrefab);

                return true;
            }
        }

        return false;
    }

	image::data_ptr image::get_image_data(unsigned char* data, int dataLength)
	{
        int channelsInFile = 0;
        return image::data_ptr(stbi_load_from_memory(data, dataLength, &m_width, &m_height, &channelsInFile, 4));
	}

	void image::bind_image_data(const image::data_ptr& imageData)
	{
        set_element_desired_size(ImVec2(m_width, m_height));

        GLuint imageTexture;
        glGenTextures(1, &imageTexture);
        glBindTexture(GL_TEXTURE_2D, imageTexture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                        imageData.get());

        m_openGlId = imageTexture;
	}

    void image::release()
    {
        if (m_openGlId != 0)
        {
            glDeleteTextures(1, &m_openGlId);
        }
    }
}