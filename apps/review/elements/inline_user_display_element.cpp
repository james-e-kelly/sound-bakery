#include "inline_user_display_element.h"

#include "windows.h"

#include "boost/algorithm/string/trim.hpp"
#include "boost/algorithm/string/case_conv.hpp"
#include "openssl/sha.h"

namespace
{
    constexpr LPCSTR g_gravatarEnvironmentVariableName = "SOUND_CHECK_GRAVATAR_API_KEY";
    constexpr std::size_t g_bufferSize                   = 1024;
    constexpr const char* g_gravatarApiUrl             = "https://api.gravatar.com/v3";
    constexpr const char* g_gravatarUrl                  = "gravatar.com";

    auto sha256_hex(const std::string& input) -> std::string
    {
        std::string email = input;
        boost::algorithm::trim(email);
        boost::algorithm::to_lower(email);

        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(email.data()), email.size(), hash);

        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        return oss.str();
    }
}

auto inline_user_display_element::render_element(const ImRect& parentRect) -> bool
{
    gluten::text userNameText("James Kelly - Needs Review", ImVec2(0.0f, 0.5f), element::anchor_preset::left_middle);
    userNameText.set_element_translation(ImVec2(parentRect.GetHeight() + ImGui::GetStyle().FramePadding.x, 0.0f));
    userNameText.render(parentRect);

    static CHAR valueBuffer[g_bufferSize] = {0};
    const DWORD size = GetEnvironmentVariable(g_gravatarEnvironmentVariableName, valueBuffer, g_bufferSize);

    if (size > 0)
    {
        static std::unique_ptr<gluten::image> userImage;

        if (userImage)
        {
            userImage->render(parentRect);
        }
        else
        {
            const std::string gravatarApiKey = valueBuffer;

            static httplib::SSLClient client(g_gravatarUrl);

            client.enable_server_hostname_verification(false);
            client.enable_server_certificate_verification(false);

            static httplib::Headers requestHeaders = {{std::string("Authorization"), gravatarApiKey}};

            std::string userEmail = "james@jameskelly.audio";
            std::string emailHash = sha256_hex(userEmail);

            const httplib::Result getProfileResult = client.Get(fmt::format("/avatar/{}?s={}&d=identicon", emailHash, parentRect.GetHeight()));

            if (getProfileResult)
            {
                std::string body = getProfileResult->body;
                int status       = getProfileResult->status;

                userImage      = std::make_unique<gluten::image>(body.c_str(), body.length());
                userImage->set_render_type(gluten::image_render::circular);
            }
            else
            {
                const std::string errMsg = httplib::to_string(getProfileResult.error());
                std::cout << errMsg << std::endl;
            }
        }
    }
    else
    {
        const DWORD error = GetLastError();
        assert(error == ERROR_ENVVAR_NOT_FOUND);
    }

    return false;
}