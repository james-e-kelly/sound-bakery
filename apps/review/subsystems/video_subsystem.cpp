#include "video_subsystem.h"

#include "gluten/subsystems/renderer_subsystem.h"
#include "sound_chef/sound_chef_internal.h"

#include "glad/include/glad/gl.h"

namespace
{
    constexpr std::string_view g_mpvDllFilename = "libmpv-2.dll";

    GLuint videoTexture = 0;
    GLuint videoFBO     = 0;
    int width = 192, height = 108;  // set to your expected video resolution
}

#define LOAD_MPV_FUNC(name) \
    name = (decltype(name))sc_dlsym(nullptr, m_mpvLibraryHandle, #name);    \
    if (!name)                                                              \
    {                                                                       \
        return 1;                                                           \
    }

video_subsystem::~video_subsystem() { BOOST_ASSERT_MSG(m_mpvLibraryHandle == nullptr, "mpv was not released"); }

static auto loop_mpv_events(mpv_handle* handle) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

    while (true)
    {
        mpv_event* ev;
        while ((ev = mpv_wait_event(handle, 0))->event_id != MPV_EVENT_NONE)
        {
            if (ev->event_id == MPV_EVENT_LOG_MESSAGE)
            {
                auto* msg = (mpv_event_log_message*)ev->data;
                const auto string = fmt::format("[{}] {}", msg->prefix, msg->text);
                std::cout << string << std::endl;
            }
        }

        using namespace std::literals::chrono_literals;
        gluten::app::get()->timer_queue()->make_delay_object(100ms, gluten::app::get()->background_executor());
    }
}

auto video_subsystem::load_video(const std::filesystem::path& absoluteFilePath) const -> void
{
    std::string file = absoluteFilePath.string();

    const char* cmd[] = {"loadfile", file.c_str(), nullptr};
    mpv_command(m_mpvHandle, cmd);

    const char* unpause[] = {"set_property", "pause", "no", nullptr};
    //mpv_command(m_mpvHandle, unpause);

    //mpv_set_property_string(m_mpvHandle, "audio", "no");
}

auto video_subsystem::get_video_texture() const -> uint32_t
{
    return videoTexture;
}

auto video_subsystem::pre_init(int ArgC, char* ArgV[]) -> int
{
    std::setlocale(LC_NUMERIC, "C");

	m_mpvLibraryHandle = sc_dlopen(nullptr, g_mpvDllFilename.data());

    if (!m_mpvLibraryHandle)
    {
        return 1;
    }

    LOAD_MPV_FUNC(mpv_create)
    LOAD_MPV_FUNC(mpv_initialize)
    LOAD_MPV_FUNC(mpv_destroy)
    LOAD_MPV_FUNC(mpv_terminate_destroy)
    LOAD_MPV_FUNC(mpv_command)
    LOAD_MPV_FUNC(mpv_get_property)
    LOAD_MPV_FUNC(mpv_set_property_string)
    LOAD_MPV_FUNC(mpv_request_log_messages)
    LOAD_MPV_FUNC(mpv_wait_event)

    LOAD_MPV_FUNC(mpv_render_context_create)
    LOAD_MPV_FUNC(mpv_render_context_render)
    LOAD_MPV_FUNC(mpv_render_context_set_update_callback)
    LOAD_MPV_FUNC(mpv_render_context_free)

    return 0;
}

typedef void* (*GLAddrLoadFunc)(const char* name);

static GLAddrLoadFunc GetGLAddrFunc() { return (GLAddrLoadFunc)gluten::renderer_subsystem::glfw_get_proc_address; }

auto video_subsystem::init() -> int
{
    if (!gladLoadGL((GLADloadfunc)GetGLAddrFunc()))
    {
        return 1;
    }

    m_mpvHandle = mpv_create();

    if (!m_mpvHandle)
    {
        return 1;
    }

    const int initResult = mpv_initialize(m_mpvHandle);

    if (initResult != 0)
    {
        return initResult;
    }

    mpv_opengl_init_params openglInitParams = 
    {
        .get_proc_address     = (void*(*)(void*, const char*)) &gluten::renderer_subsystem::glfw_get_proc_address_with_context,
        .get_proc_address_ctx = nullptr,
    };

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &openglInitParams},
        //{MPV_RENDER_PARAM_ADVANCED_CONTROL, (void*)1},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };

    const int renderContextCreateResult = mpv_render_context_create(&m_mpvRenderContext, m_mpvHandle, params);

    if (renderContextCreateResult != 0 || m_mpvRenderContext != nullptr)
    {
        return renderContextCreateResult;
    }

    mpv_render_context_set_update_callback(m_mpvRenderContext, [](void* ctx){}, nullptr);

    //mpv_request_log_messages(m_mpvHandle, "info");

    glGenTextures(1, &videoTexture);
    assert(videoTexture != 0);

    glBindTexture(GL_TEXTURE_2D, videoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &videoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, videoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, videoTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        return 1;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}

auto video_subsystem::tick(double deltaTime) -> void
{
}

auto video_subsystem::tick_rendering(double deltaTime) -> void
{
    glBindFramebuffer(GL_FRAMEBUFFER, videoFBO);
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    mpv_opengl_fbo fbo = {
        .fbo             = static_cast<int>(videoFBO),
        .w               = width,
        .h               = height,
        .internal_format = GL_RGBA,
    };

    const bool flipY = true;

    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &fbo}, 
        {MPV_RENDER_PARAM_FLIP_Y, (void*)&flipY},
        {MPV_RENDER_PARAM_INVALID, nullptr}};

    const int renderErrorCode = mpv_render_context_render(m_mpvRenderContext, params);
    assert(renderErrorCode == 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(videoTexture)),
    //             ImVec2((float)width, (float)height), ImVec2(0, 1),  // flip vertically
    //             ImVec2(1, 0));
}

auto video_subsystem::exit() -> void
{
    if (m_mpvRenderContext)
    {
        mpv_render_context_free(m_mpvRenderContext);
        m_mpvRenderContext = nullptr;
    }

    if (m_mpvHandle)
    {
        mpv_terminate_destroy(m_mpvHandle);
        m_mpvHandle = nullptr;
    }

    if (m_mpvLibraryHandle)
    {
        sc_dlclose(nullptr, m_mpvLibraryHandle);
        m_mpvLibraryHandle = nullptr;
    }
}
