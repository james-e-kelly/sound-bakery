#include "splash_widget.h"

#include "app/app.h"
#include "imgui.h"
#include "stb_image.h"

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(sbk::Images);

namespace SplashScreenUtils
{
    static const char* splashScreenName = "SplashScreen";
}

void splash_widget::start()
{
    widget::start();

    const cmrc::embedded_filesystem embeddedfilesystem = cmrc::sbk::Images::get_filesystem();

    const cmrc::file splashImageFile = embeddedfilesystem.open("SplashImage02.png");

    int image_width, image_height, n;
    unsigned char* image_data = stbi_load_from_memory((stbi_uc*)splashImageFile.begin(), splashImageFile.size(),
                                                      &image_width, &image_height, &n, 0);

    assert(image_data);

    if (image_data != NULL)
    {
        // Create a OpenGL texture identifier
        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        m_splashImageID     = image_texture;
        m_splashImageWidth  = image_width;
        m_splashImageHeight = image_height;
    }
}

void splash_widget::show_splash_screen()
{
    m_wantsToShow       = true;
    m_timeShowingScreen = 0.0;
}

void splash_widget::close_splash_screen() { m_wantsToShow = false; }

void splash_widget::tick(double deltaTime)
{
    if (m_wantsToShow)
    {
        m_timeShowingScreen += deltaTime;

        static int previousRoundedTime = 0;
        int roundedTime                = static_cast<int>(m_timeShowingScreen / 0.2) % 4;

        if (roundedTime != previousRoundedTime)
        {
            switch (roundedTime)
            {
                case 0:
                    m_loadingText = std::string("Loading");
                    break;

                case 1:
                    m_loadingText = std::string("Loading.");
                    break;

                case 2:
                    m_loadingText = std::string("Loading..");
                    break;

                case 3:
                    m_loadingText = std::string("Loading...");
                    break;
            }
            previousRoundedTime = roundedTime;
        }

        if (m_timeShowingScreen >= 2.0)
        {
            destroy();
        }
    }
}

void splash_widget::render()
{
    if (m_wantsToShow && !ImGui::IsPopupOpen(SplashScreenUtils::splashScreenName))
    {
        ImGui::OpenPopup(SplashScreenUtils::splashScreenName);
    }

    // Always center this window when appearing
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(SplashScreenUtils::splashScreenName, NULL,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::Image((void*)(intptr_t)m_splashImageID, ImVec2(m_splashImageWidth, m_splashImageHeight));

        ImGui::ProgressBar(-1.0f * (float)ImGui::GetTime(), ImVec2(m_splashImageWidth, 0.0f),
                           "Loading...");

        ImGui::EndPopup();
    }
}
