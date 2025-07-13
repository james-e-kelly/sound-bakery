#include "inline_user_display_element.h"

#include "windows.h"

namespace
{
    constexpr LPCSTR g_gravatarEnvironmentVariableName = "SOUND_CHECK_GRAVATAR_API_KEY";
    constexpr std::size_t g_bufferSize                   = 1024;
    constexpr const char* g_gravatarApiUrl             = "https://api.gravatar.com/v3";
}

auto inline_user_display_element::render_element(const ImRect& parentRect) -> bool
{
    gluten::text userNameText("James Kelly", ImVec2(0.0f, 0.5f), element::anchor_preset::left_middle);
    userNameText.render(parentRect);

    static CHAR valueBuffer[g_bufferSize] = {0};
    const DWORD result = GetEnvironmentVariable(g_gravatarEnvironmentVariableName, valueBuffer, g_bufferSize);

    if (result == S_OK)
    {
        const std::string gravatarApiKey = valueBuffer;
    }

    return false;
}