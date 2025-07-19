#include "inline_user_display_element.h"

#include "windows.h"

#include "app/review_app.h"
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

    class avatar_resolver
    {
    public:
        avatar_resolver()
        {
            client = std::make_shared<httplib::SSLClient>(g_gravatarUrl);
            client->enable_server_hostname_verification(false);
            client->enable_server_certificate_verification(false);
        }

        auto get_avatar_image(const std::string& email, float size) -> gluten::image*
        {
            if (m_cache.contains(email))
            {
                return m_cache.at(email).get();
            }
            else if (m_loading.contains(email))
            {
                if (m_loading.at(email).status() == concurrencpp::result_status::value)
                {
                    m_cache.insert({email, m_loading.at(email).get()});
                }
            }
            else
            {
                concurrencpp::result<std::unique_ptr<gluten::image>> result = load(email, size);
                m_loading.insert({email, std::move(result)});
            }
            return nullptr;
        }


    private:
        auto load(const std::string email, float size) -> concurrencpp::result<std::unique_ptr<gluten::image>>
        {
            co_await concurrencpp::resume_on(review_app::get()->background_executor());

            const std::string emailHash = sha256_hex(email);

            const httplib::Result result = client->Get(fmt::format("/avatar/{}?s={}&d=identicon", emailHash, size));

            if (result)
            {
                const std::string body = result->body;

                co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());

                std::unique_ptr<gluten::image> avatarImage = std::make_unique<gluten::image>(body.c_str(), body.length());
                avatarImage->set_render_type(gluten::image_render::circular);
                co_return std::move(avatarImage);
            }
            co_return std::unique_ptr<gluten::image>{};
        }

        std::shared_ptr<httplib::SSLClient> client;

        std::unordered_map<std::string, std::unique_ptr<gluten::image>> m_cache;
        std::unordered_map<std::string, concurrencpp::result<std::unique_ptr<gluten::image>>> m_loading;
    };
}

auto inline_user_display_element::render_element(const ImRect& parentRect) -> bool
{
    static avatar_resolver resolver;

    if (gluten::image* avatarImage = resolver.get_avatar_image(m_userEmailAddress, parentRect.GetHeight()))
    {
        gluten::text userNameText(fmt::format("{} - Needs Review", m_userDisplayName), ImVec2(0.0f, 0.5f),
                                  element::anchor_preset::left_middle);
        userNameText.set_element_translation(ImVec2(parentRect.GetHeight() + ImGui::GetStyle().FramePadding.x, 0.0f));
        userNameText.render(parentRect);

        avatarImage->render(parentRect);
    }
    else
    {
        ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);
    }

    return false;
}