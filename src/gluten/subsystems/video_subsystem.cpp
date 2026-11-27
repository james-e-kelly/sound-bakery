#include "video_subsystem.h"

#if GLUTEN_ENABLE_VIDEO

    #include "glad/include/glad/gl.h"
    #include "gluten/app/app.h"
    #include "gluten/subsystems/audio_subsystem.h"
    #include "gluten/subsystems/renderer_subsystem.h"
    #include "sound_chef/sound_chef.h"

    #ifdef _WIN32
        #define WIN32_LEAN_AND_MEAN
        #include <Windows.h>
        #undef WIN32_LEAN_AND_MEAN
    #endif

namespace
{
    constexpr std::string_view g_mpvDllFilename = "libmpv-2.dll";
    constexpr int32_t g_mpvAdvancedControl      = 1;
    constexpr float g_maxTextureDimension       = 1920.0f;
    constexpr double g_endOfFileTolerance       = 0.05;

    mpv_opengl_init_params openglInitParams =
        {
            .get_proc_address     = (void* (*)(void*, const char*)) & gluten::renderer_subsystem::glfw_get_proc_address_with_context,
            .get_proc_address_ctx = nullptr,
    };

    mpv_render_param g_mpvRenderParams[] =
        {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &openglInitParams},
            {MPV_RENDER_PARAM_ADVANCED_CONTROL, (void*)&g_mpvAdvancedControl},
            {MPV_RENDER_PARAM_INVALID, nullptr},
    };
}  // namespace

gluten::video_subsystem::mpv_context::mpv_context()
{
    m_mpvHandle = mpv_create();
    mpv_set_property_string(m_mpvHandle, "vo", "libmpv");
    mpv_set_property_string(m_mpvHandle, "keep-open", "yes");              // Hold the last frame at the end instead of unloading
    mpv_set_property_string(m_mpvHandle, "audio-pitch-correction", "no");  // Speed changes pitch, as it does for audio files
    mpv_set_property_string(m_mpvHandle, "volume-max", "200");             // Room for the preview fader's +12 dB
    mpv_initialize(m_mpvHandle);
    mpv_request_log_messages(m_mpvHandle, "info");
    mpv_observe_property(m_mpvHandle, 0, "duration", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "time-pos", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "pause", MPV_FORMAT_FLAG);
    mpv_observe_property(m_mpvHandle, 0, "container-fps", MPV_FORMAT_DOUBLE);
    mpv_observe_property(m_mpvHandle, 0, "dwidth", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpvHandle, 0, "dheight", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpvHandle, 0, "video-format", MPV_FORMAT_STRING);
    mpv_observe_property(m_mpvHandle, 0, "audio-codec-name", MPV_FORMAT_STRING);
    mpv_observe_property(m_mpvHandle, 0, "audio-params/samplerate", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpvHandle, 0, "audio-params/channel-count", MPV_FORMAT_INT64);
    mpv_observe_property(m_mpvHandle, 0, "file-size", MPV_FORMAT_INT64);

    mpv_render_context_create(&m_mpvRenderContext, m_mpvHandle, g_mpvRenderParams);

    mpv_render_context_set_update_callback(m_mpvRenderContext, [](void* context)
                                           {
            mpv_context* const mpvContext = reinterpret_cast<mpv_context*>(context);
            mpvContext->m_needsRender.store(true); }, this);

    glGenTextures(1, &videoTexture);
    assert(videoTexture != 0);

    glBindTexture(GL_TEXTURE_2D, videoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_displaySize.x, m_displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &videoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, videoFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, videoTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

gluten::video_subsystem::mpv_context::~mpv_context()
{
    if (m_waitEventResult)
    {
        m_waitEventResult.get();
    }

    if (m_mpvRenderContext)
    {
        mpv_render_context_free(m_mpvRenderContext);
    }

    if (m_mpvHandle)
    {
        mpv_terminate_destroy(m_mpvHandle);
    }
}

    /**
     * @def Loads an mpv symbol pointer so functions can be used.
     */
    #define LOAD_MPV_SYMBOL(name)                                            \
        name = (decltype(name))sc_dlsym(nullptr, m_mpvLibraryHandle, #name); \
        if (!name)                                                           \
        {                                                                    \
            return 1;                                                        \
        }

gluten::video_subsystem::~video_subsystem() { BOOST_ASSERT_MSG(m_mpvLibraryHandle == nullptr, "mpv was not released"); }

auto gluten::video_subsystem::load_video(const std::filesystem::path& absoluteFilePath) -> void
{
    std::unique_ptr<mpv_context> mpvContext = std::make_unique<mpv_context>();

    const std::string file = absoluteFilePath.string();
    const char* cmd[]      = {"loadfile", file.c_str(), "replace", nullptr};
    mpv_command(mpvContext->m_mpvHandle, cmd);
    mpv_command_string(mpvContext->m_mpvHandle, "set pause yes");

    m_videoFileToContexts.insert({file, mpvContext->m_mpvHandle});
    m_mpvContexts.insert({mpvContext->m_mpvHandle, std::move(mpvContext)});
}

auto gluten::video_subsystem::get_video_texture(const std::filesystem::path& file) const -> uint32_t
{
    if (mpv_handle* handle = get_mpv_handle_from_file(file))
    {
        if (m_mpvContexts.contains(handle))
        {
            if (m_mpvContexts.at(handle))
            {
                return m_mpvContexts.at(handle)->videoTexture;
            }
        }
    }

    return 0U;
}

auto gluten::video_subsystem::play_video(const std::filesystem::path& absoluteFilePath) -> void
{
    // Pause everything so there is only one video playing at once
    for (const auto& handle : m_videoFileToContexts)
    {
        if (handle.second)
        {
            mpv_command_string(handle.second, "set pause yes");

            if (m_mpvContexts.contains(handle.second))
            {
                m_mpvContexts.at(handle.second)->m_playing = false;
            }
        }
    }

    if (mpv_handle* const handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        if (m_mpvContexts.contains(handle))
        {
            mpv_context& context = *m_mpvContexts.at(handle);

            // keep-open parks the file on its last frame, so playing from there starts over like the audio preview does.
            if (context.m_videoDuration > 0.0 && context.m_playPosition >= context.m_videoDuration - g_endOfFileTolerance)
            {
                double start = 0.0;
                mpv_set_property(handle, "time-pos", MPV_FORMAT_DOUBLE, &start);
            }

            context.m_playing = true;
        }

        mpv_command_string(handle, "set pause no");
    }
}

auto gluten::video_subsystem::pause_video(const std::filesystem::path& absoluteFilePath) -> void
{
    if (mpv_handle* const handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        mpv_command_string(handle, "set pause yes");

        if (m_mpvContexts.contains(handle))
        {
            m_mpvContexts.at(handle)->m_playing = false;
        }
    }
}

auto gluten::video_subsystem::get_video_play_position(const std::filesystem::path& absoluteFilePath) -> double
{
    mpv_handle* handle = get_mpv_handle_from_file(absoluteFilePath);

    if (m_mpvContexts.contains(handle))
    {
        if (const auto& context = m_mpvContexts.at(handle))
        {
            return context->m_playPosition;
        }
    }

    return 0.0;
}

auto gluten::video_subsystem::get_video_duration(const std::filesystem::path& absoluteFilePath) -> double
{
    mpv_handle* handle = get_mpv_handle_from_file(absoluteFilePath);

    if (m_mpvContexts.contains(handle))
    {
        if (const auto& context = m_mpvContexts.at(handle))
        {
            return context->m_videoDuration;
        }
    }

    return 0.1;  // Ensure there is slightly more duration than play position to stop divide by zeroes
}

auto gluten::video_subsystem::set_video_play_position(const std::filesystem::path& absoluteFilePath, double position) -> void
{
    if (mpv_handle* handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        mpv_set_property(handle, "time-pos", MPV_FORMAT_DOUBLE, &position);
    }
}

auto gluten::video_subsystem::set_video_next_frame(const std::filesystem::path& absoluteFilePath) -> void
{
    if (mpv_handle* handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        const char* cmd[] = {"frame-step", nullptr};
        mpv_command(handle, cmd);
    }
}

auto gluten::video_subsystem::set_video_prev_frame(const std::filesystem::path& absoluteFilePath) -> void
{
    if (mpv_handle* handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        const char* cmd[] = {"frame-back-step", nullptr};
        mpv_command(handle, cmd);
    }
}

auto gluten::video_subsystem::get_video_is_playing(const std::filesystem::path& absoluteFilePath) const -> bool
{
    bool playing = false;

    if (mpv_handle* const handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        if (m_mpvContexts.contains(handle))
        {
            playing = m_mpvContexts.at(handle)->m_playing;
        }
    }

    return playing;
}

auto gluten::video_subsystem::get_video_info(const std::filesystem::path& absoluteFilePath) const -> const video_info*
{
    if (mpv_handle* const handle = get_mpv_handle_from_file(absoluteFilePath))
    {
        if (const auto context = m_mpvContexts.find(handle); context != m_mpvContexts.end() && context->second)
        {
            return &context->second->m_info;
        }
    }

    return nullptr;
}

auto gluten::video_subsystem::stop_all_videos() const -> void
{
    for (const auto& handle : m_videoFileToContexts)
    {
        mpv_command_string(handle.second, "set pause yes");

        if (m_mpvContexts.contains(handle.second))
        {
            m_mpvContexts.at(handle.second)->m_playing = false;
        }
    }
}

auto gluten::video_subsystem::pre_init(const boost::program_options::variables_map& cliVariables) -> int
{
    std::setlocale(LC_NUMERIC, "C");

    m_mpvLibraryHandle = sc_dlopen(nullptr, g_mpvDllFilename.data());

    if (!m_mpvLibraryHandle)
    {
        return 1;
    }

    LOAD_MPV_SYMBOL(mpv_create)
    LOAD_MPV_SYMBOL(mpv_initialize)
    LOAD_MPV_SYMBOL(mpv_destroy)
    LOAD_MPV_SYMBOL(mpv_terminate_destroy)
    LOAD_MPV_SYMBOL(mpv_command)
    LOAD_MPV_SYMBOL(mpv_get_property)
    LOAD_MPV_SYMBOL(mpv_set_property)
    LOAD_MPV_SYMBOL(mpv_set_property_string)
    LOAD_MPV_SYMBOL(mpv_request_log_messages)
    LOAD_MPV_SYMBOL(mpv_wait_event)
    LOAD_MPV_SYMBOL(mpv_observe_property)
    LOAD_MPV_SYMBOL(mpv_command_string)

    LOAD_MPV_SYMBOL(mpv_render_context_create)
    LOAD_MPV_SYMBOL(mpv_render_context_render)
    LOAD_MPV_SYMBOL(mpv_render_context_set_update_callback)
    LOAD_MPV_SYMBOL(mpv_render_context_free)
    LOAD_MPV_SYMBOL(mpv_render_context_update)

    return 0;
}

typedef void* (*GLAddrLoadFunc)(const char* name);

static GLAddrLoadFunc GetGLAddrFunc() { return (GLAddrLoadFunc)gluten::renderer_subsystem::glfw_get_proc_address; }

auto gluten::video_subsystem::init() -> int
{
    if (!gladLoadGL((GLADloadfunc)GetGLAddrFunc()))
    {
        return 1;
    }

    return 0;
}

auto gluten::video_subsystem::tick(double deltaTime) -> void
{
    apply_preview_settings();

    static double elapsed = 0.0;

    if ((elapsed += deltaTime) > 0.05)
    {
        for (auto& context : m_mpvContexts)
        {
            if (context.second && (!context.second->m_waitEventResult || context.second->m_waitEventResult.status() == concurrencpp::result_status::value))
            {
                context.second->m_waitEventResult = wait_for_mpv_events(context.second->m_mpvHandle);
            }
        }
    }
}

auto gluten::video_subsystem::tick_rendering(double deltaTime) -> void
{
    for (auto& mpvContext : m_mpvContexts)
    {
        std::unique_ptr<mpv_context>& mpvContextPtr = mpvContext.second;

        if (!mpvContextPtr)
        {
            continue;
        }

        // Render at the video's own display size (capped) so the texture's aspect matches the picture.
        if (mpvContextPtr->m_info.displayWidth > 0 && mpvContextPtr->m_info.displayHeight > 0)
        {
            const float width  = static_cast<float>(mpvContextPtr->m_info.displayWidth);
            const float height = static_cast<float>(mpvContextPtr->m_info.displayHeight);
            const float scale  = (std::min)(1.0f, g_maxTextureDimension / (std::max)(width, height));  // Parenthesised against Windows.h min/max
            const ImVec2 textureSize(std::round(width * scale), std::round(height * scale));

            if (textureSize.x != mpvContextPtr->m_displaySize.x || textureSize.y != mpvContextPtr->m_displaySize.y)
            {
                mpvContextPtr->m_displaySize = textureSize;

                glBindTexture(GL_TEXTURE_2D, mpvContextPtr->videoTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(textureSize.x), static_cast<GLsizei>(textureSize.y), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glBindTexture(GL_TEXTURE_2D, 0);

                mpvContextPtr->m_forceRender = true;
            }
        }

        const bool forceRender = std::exchange(mpvContextPtr->m_forceRender, false);

        if (mpvContextPtr->m_needsRender.exchange(false) || forceRender)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, mpvContextPtr->videoFBO);
            glViewport(0, 0, static_cast<GLsizei>(mpvContextPtr->m_displaySize.x), static_cast<GLsizei>(mpvContextPtr->m_displaySize.y));

            mpv_opengl_fbo fbo = {
                .fbo             = static_cast<int>(mpvContextPtr->videoFBO),
                .w               = static_cast<int>(mpvContextPtr->m_displaySize.x),
                .h               = static_cast<int>(mpvContextPtr->m_displaySize.y),
                .internal_format = GL_RGBA,
            };

            const uint64_t updateResult = mpv_render_context_update(mpvContextPtr->m_mpvRenderContext);

            if ((updateResult & MPV_RENDER_UPDATE_FRAME) != 0 || forceRender)
            {
                const int flipY           = 0;
                mpv_render_param params[] = {{MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
                                             {MPV_RENDER_PARAM_FLIP_Y, (void*)&flipY},
                                             {MPV_RENDER_PARAM_INVALID, nullptr}};

                const int renderErrorCode = mpv_render_context_render(mpvContextPtr->m_mpvRenderContext, params);
                assert(renderErrorCode == 0);
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }
}

auto gluten::video_subsystem::exit() -> void
{
    m_mpvContexts.clear();
    m_videoFileToContexts.clear();

    if (m_mpvLibraryHandle)
    {
        sc_dlclose(nullptr, m_mpvLibraryHandle);
        m_mpvLibraryHandle = nullptr;
    }
}

auto gluten::video_subsystem::set_video_play_position(mpv_handle* handle, double playPosition) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (m_mpvContexts.contains(handle))
    {
        m_mpvContexts.at(handle)->m_playPosition = playPosition;
    }
}

auto gluten::video_subsystem::set_video_duration(mpv_handle* handle, double duration) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (m_mpvContexts.contains(handle))
    {
        m_mpvContexts.at(handle)->m_videoDuration = duration;
    }
}

auto gluten::video_subsystem::update_context(mpv_handle* handle, std::function<void(mpv_context&)> update) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (const auto context = m_mpvContexts.find(handle); context != m_mpvContexts.end() && context->second)
    {
        update(*context->second);
    }
}

auto gluten::video_subsystem::apply_preview_settings() -> void
{
    std::shared_ptr<audio_subsystem> audioSubsystem = get_app()->get_subsystem_by_class<audio_subsystem>();
    if (!audioSubsystem)
    {
        return;
    }

    // mpv's software volume is cubic, so the cube root turns the preview's linear gain into its percentage.
    const float volume = 100.0f * std::cbrt(audioSubsystem->get_preview_volume());
    const float speed  = audioSubsystem->get_preview_speed();
    const bool looping = audioSubsystem->get_preview_looping();

    for (auto& [handle, context] : m_mpvContexts)
    {
        if (!context)
        {
            continue;
        }

        mpv_context::applied_preview_settings& applied = context->m_appliedPreview;

        if (applied.volume != volume)
        {
            double mpvVolume = volume;
            mpv_set_property(handle, "volume", MPV_FORMAT_DOUBLE, &mpvVolume);
            applied.volume = volume;
        }

        if (applied.speed != speed)
        {
            double mpvSpeed = speed;
            mpv_set_property(handle, "speed", MPV_FORMAT_DOUBLE, &mpvSpeed);
            applied.speed = speed;
        }

        if (applied.looping != looping)
        {
            mpv_set_property_string(handle, "loop-file", looping ? "inf" : "no");
            applied.looping = looping;
        }
    }
}

auto gluten::video_subsystem::set_video_end(mpv_handle* handle) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(get_app()->get_tick_executor());

    if (m_mpvContexts.contains(handle))
    {
        m_mpvContexts.erase(handle);

        m_videoFileToContexts.erase(std::find_if(m_videoFileToContexts.begin(), m_videoFileToContexts.end(), [handle](const auto& pair)
                                                 { return pair.second == handle; }));
    }
}

auto gluten::video_subsystem::wait_for_mpv_events(mpv_handle* handle) -> concurrencpp::result<void>
{
    if (handle == nullptr)
    {
        co_return;
    }

    co_await concurrencpp::resume_on(get_app()->thread_pool_executor());

    for (const mpv_event* event = mpv_wait_event(handle, 0); event != nullptr; event = mpv_wait_event(handle, 0))
    {
        if (event->event_id == MPV_EVENT_NONE)
        {
            break;
        }

        if (event->data == nullptr)
        {
            continue;
        }

        if (event->event_id == MPV_EVENT_LOG_MESSAGE)
        {
            mpv_event_log_message* logMessage = static_cast<mpv_event_log_message*>(event->data);
            const std::string logString       = fmt::format("[{}] {}", logMessage->prefix, logMessage->text);
            OutputDebugString(logString.c_str());
        }
        else if (event->event_id == MPV_EVENT_FILE_LOADED)
        {
        }
        else if (event->event_id == MPV_EVENT_PROPERTY_CHANGE)
        {
            const mpv_event_property* property = (mpv_event_property*)event->data;
            if (strcmp(property->name, "time-pos") == 0)
            {
                if (property->format == MPV_FORMAT_DOUBLE)
                {
                    const double time = *static_cast<double*>(property->data);
                    set_video_play_position(handle, time);
                }
            }
            else if (strcmp(property->name, "duration") == 0)
            {
                if (property->format == MPV_FORMAT_DOUBLE)
                {
                    double time = *static_cast<double*>(property->data);
                    set_video_duration(handle, time);
                }
            }
            else if (property->format == MPV_FORMAT_FLAG && strcmp(property->name, "pause") == 0)
            {
                const bool paused = *static_cast<int*>(property->data) != 0;
                update_context(handle, [paused](mpv_context& context)
                               { context.m_playing = !paused; });
            }
            else if (property->format == MPV_FORMAT_DOUBLE && strcmp(property->name, "container-fps") == 0)
            {
                const double frameRate = *static_cast<double*>(property->data);
                update_context(handle, [frameRate](mpv_context& context)
                               { context.m_info.frameRate = frameRate; });
            }
            else if (property->format == MPV_FORMAT_INT64)
            {
                const int64_t value = *static_cast<int64_t*>(property->data);
                const std::string name(property->name);

                update_context(handle, [value, name](mpv_context& context)
                               {
                                   if (name == "dwidth") { context.m_info.displayWidth = value; }
                                   else if (name == "dheight") { context.m_info.displayHeight = value; }
                                   else if (name == "audio-params/samplerate") { context.m_info.audioSampleRate = value; }
                                   else if (name == "audio-params/channel-count") { context.m_info.audioChannels = value; }
                                   else if (name == "file-size") { context.m_info.fileSize = value; } });
            }
            else if (property->format == MPV_FORMAT_STRING)
            {
                const char* const* text = static_cast<const char* const*>(property->data);
                const std::string value(*text ? *text : "");
                const bool isVideo = strcmp(property->name, "video-format") == 0;

                update_context(handle, [value, isVideo](mpv_context& context)
                               { (isVideo ? context.m_info.videoCodec : context.m_info.audioCodec) = value; });
            }
        }
        else if (event->event_id == MPV_EVENT_END_FILE)
        {
            set_video_end(handle);
        }
        else if (event->event_id == MPV_EVENT_SHUTDOWN)
        {
            co_return;
        }
    }

    co_return;
}

auto gluten::video_subsystem::get_mpv_handle_from_file(const std::filesystem::path& absoluteFilePath) const -> mpv_handle*
{
    if (m_videoFileToContexts.contains(absoluteFilePath.string()))
    {
        return m_videoFileToContexts.at(absoluteFilePath.string());
    }

    return 0U;
}

#else  // GLUTEN_ENABLE_VIDEO

// libmpv was not available at configure time, so the video subsystem is built
// as a no-op. The rest of gluten (and any app that instantiates it) still
// compiles and links against the same public interface; video playback is
// simply unavailable and every call is a safe default.

gluten::video_subsystem::~video_subsystem() = default;

auto gluten::video_subsystem::load_video(const std::filesystem::path&) -> void {}
auto gluten::video_subsystem::get_video_texture(const std::filesystem::path&) const -> uint32_t { return 0U; }
auto gluten::video_subsystem::play_video(const std::filesystem::path&) -> void {}
auto gluten::video_subsystem::pause_video(const std::filesystem::path&) -> void {}
auto gluten::video_subsystem::get_video_play_position(const std::filesystem::path&) -> double { return 0.0; }
// Slightly above the play position so consumers dividing by duration don't hit a divide-by-zero.
auto gluten::video_subsystem::get_video_duration(const std::filesystem::path&) -> double { return 0.1; }
auto gluten::video_subsystem::set_video_play_position(const std::filesystem::path&, double) -> void {}
auto gluten::video_subsystem::set_video_next_frame(const std::filesystem::path&) -> void {}
auto gluten::video_subsystem::set_video_prev_frame(const std::filesystem::path&) -> void {}
auto gluten::video_subsystem::get_video_is_playing(const std::filesystem::path&) const -> bool { return false; }
auto gluten::video_subsystem::get_video_info(const std::filesystem::path&) const -> const video_info* { return nullptr; }
auto gluten::video_subsystem::stop_all_videos() const -> void {}
auto gluten::video_subsystem::pre_init(const boost::program_options::variables_map&) -> int { return 0; }
auto gluten::video_subsystem::init() -> int { return 0; }
auto gluten::video_subsystem::tick(double) -> void {}
auto gluten::video_subsystem::tick_rendering(double) -> void {}
auto gluten::video_subsystem::exit() -> void {}

#endif  // GLUTEN_ENABLE_VIDEO
