#include "inline_user_display_element.h"

#include "windows.h"

#include "app/review_app.h"
#include "managers/workspace_manager.h"

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

    struct avatar_identifier
    {
        std::string m_email;
        float m_size                  = 10.0f;
        gluten::image_render m_render = gluten::image_render::circular;

        bool operator<(const avatar_identifier& rhs) const
        {
            return m_email < rhs.m_email && m_size < rhs.m_size && m_render < rhs.m_render;
        }

        bool operator==(const avatar_identifier& rhs) const
        {
            return m_email == rhs.m_email && std::abs(m_size - rhs.m_size) < 32.0f && m_render == rhs.m_render;
        }
    };

    struct avatar_indentifier_hasher
    {
        std::size_t operator()(const avatar_identifier& k) const
        {
            using std::hash;
            using std::size_t;
            using std::string;
            return ((hash<string>()(k.m_email) ^ (hash<float>()(k.m_size) << 1)) >> 1) ^
                   (hash<int>()((int)k.m_render) << 1);
        }
    };

    class avatar_resolver
    {
    public:
        avatar_resolver()
        {
            client = std::make_shared<httplib::SSLClient>(g_gravatarUrl);
            client->enable_server_hostname_verification(false);
            client->enable_server_certificate_verification(false);
        }

        auto get_avatar_image(const std::string& email, float size, gluten::image_render render) -> gluten::image*
        {
            const avatar_identifier avatar{.m_email = email, .m_size = size, .m_render = render};

            if (m_cache.get_cache_needs_filling(avatar))
            {
                m_cache.set_async_fill_cache(avatar, load(email, size, render));
            }

            return m_cache.get_cached_data(avatar).m_cache.get();
        }

    private:
        auto load(const std::string email, float size, gluten::image_render render) -> concurrencpp::result<std::unique_ptr<gluten::image>>
        {
            co_await concurrencpp::resume_on(review_app::get()->background_executor());

            const std::string emailHash = sha256_hex(email);

            const httplib::Result result = client->Get(fmt::format("/avatar/{}?s={}&d=identicon", emailHash, size));

            if (result)
            {
                const std::string body = result->body;

                co_await concurrencpp::resume_on(review_app::get()->get_tick_executor());

                std::unique_ptr<gluten::image> avatarImage = std::make_unique<gluten::image>(body.c_str(), body.length());
                avatarImage->set_render_type(render);
                avatarImage->set_image_size(ImVec2(size, size));
                co_return std::move(avatarImage);
            }
            co_return std::unique_ptr<gluten::image>{};
        }

        std::shared_ptr<httplib::SSLClient> client;

        gluten::data_cache<std::unique_ptr<gluten::image>, avatar_identifier, avatar_indentifier_hasher> m_cache;
    };
}

auto user_avatar_element::set_avatar_render(gluten::image_render render) -> void { m_render = render; }

auto user_avatar_element::render_element(const ImRect& parentRect) -> bool
{
    static avatar_resolver resolver;

    if (avatarImage = resolver.get_avatar_image(m_userEmailAddress, std::abs(parentRect.GetHeight()), m_render))
    {
        avatarImage->render(parentRect);
        return true;
    }
    else
    {
        ImSpinner::SpinnerAngEclipse("##Loading", ImGui::GetFontSize() / 2.0f, 2.0f, gluten::theme::white, 8.0f);
        return false;
    }
}

auto logged_in_user_element::render_element(const ImRect& parentRect) -> bool
{
    user_avatar_element avatar(m_userEmailAddress);
    avatar.set_element_frame_padding();
    avatar.render(parentRect);

    gluten::text emailText(m_userEmailAddress, ImVec2(0.0f, 0.5f), element::anchor_preset::left_middle);
    emailText.set_element_translation(ImVec2(-emailText.get_element_content_size(parentRect.GetSize()).x, 0.0f));
    emailText.render(parentRect);

    return false;
}

auto reviewer_display_element::render_element(const ImRect& parentRect) -> bool
{
    gluten::imgui::scoped_id id(m_userEmailAddress.c_str());

    const bool renderingLoggedInUser = m_userSettings->m_loggedInUser.m_email == m_userEmailAddress;

    user_avatar_element avatar(m_userEmailAddress);
    avatar.set_element_padding(ImVec2(5.0f, 0.0f));
    if (avatar.render(parentRect))
    {
        const std::string tooltipText = fmt::format("{} {}", m_userDisplayName,
                                                    m_vote == review_vote::no_vote  ? "No Vote"
                                                    : m_vote == review_vote::upvote ? ICON_LC_THUMBS_UP
                                                                                    : ICON_LC_THUMBS_DOWN);

        if (ImDrawList* const drawList = ImGui::GetForegroundDrawList())
        {
            const ImU32 circleColor =
                ImGui::GetColorU32(m_vote == review_vote::upvote     ? gluten::theme::carbon_g100::supportSuccess
                                    : m_vote == review_vote::downvote ? gluten::theme::carbon_g100::supportError
                                                                        : gluten::theme::carbon_g100::layer03);

            drawList->AddCircle(avatar.get_image_rect().GetCenter(), avatar.get_element_rect().GetSize().x / 2.0f, circleColor, 0, m_vote == review_vote::no_vote ? 2.0f : 4.0f);
        }

        ImGui::SetItemTooltip(tooltipText.c_str());
    }

    return false;
}