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

    void image::render()
    {
        if (m_openGlId != 0 && m_width > 0 && m_height > 0)
        {
            if (ImDrawList* const drawList = ImGui::GetForegroundDrawList())
            {
                drawList->AddImage((void*)(intptr_t)m_openGlId, get_element_start(), get_element_end());
               
                //ImGui::DebugDrawItemRect(gluten::theme::invalidPrefab);
            }
        }
    }

	image::data_ptr image::get_image_data(unsigned char* data, int dataLength)
	{
        int channelsInFile = 0;
        return image::data_ptr(stbi_load_from_memory(data, dataLength, &m_width, &m_height, &channelsInFile, 4));
	}

	void image::bind_image_data(const image::data_ptr& imageData)
	{
        set_element_size(ImVec2(m_width, m_height));

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