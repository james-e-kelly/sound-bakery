#include "audio_element.h"

#include "gluten/app/app.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"
#include "implot.h"
#include "implot_internal.h"

namespace
{
    constexpr float s_laneHalfHeight = 1.1f;  //< Lanes are 2.2 units tall so +-1.0 peaks keep a little headroom
    constexpr float s_laneStride     = s_laneHalfHeight * 2.0f;

    constexpr float s_lufsAxisMin     = -48.0f;
    constexpr float s_lufsAxisMax     = 0.0f;
    constexpr float s_bandsAxisMin    = -96.0f;
    constexpr float s_clipThreshold   = 0.999f;
    constexpr float s_silenceLoudness = -150.0f;  //< libebur128 reports silence as a huge negative number

    auto lane_center(std::size_t channel) -> float { return -s_laneStride * static_cast<float>(channel); }

    auto max_digit_advance(ImFont* font, float fontSize) -> float
    {
        float width = 0.0f;
        for (char digit = '0'; digit <= '9'; ++digit)
        {
            width = std::max(width, font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, &digit, &digit + 1).x);
        }
        return width;
    }

    struct ruler_step
    {
        double major;
        int minorDivisions;
    };

    constexpr ruler_step s_rulerSteps[] = {{0.001, 5}, {0.005, 5}, {0.01, 5}, {0.05, 5}, {0.1, 5}, {0.25, 5}, {0.5, 5}, {1.0, 5}, {2.0, 4}, {5.0, 5}, {10.0, 10}, {15.0, 3}, {30.0, 6}, {60.0, 6}, {120.0, 4}, {300.0, 5}, {600.0, 10}};

    auto pick_ruler_step(double pixelsPerSecond, float labelWidth) -> ruler_step
    {
        for (const ruler_step& step : s_rulerSteps)
        {
            if (step.major * pixelsPerSecond >= labelWidth)
            {
                return step;
            }
        }
        return s_rulerSteps[std::size(s_rulerSteps) - 1];
    }

    auto channel_label(std::size_t channel, std::size_t channels, bool midSide) -> std::string
    {
        if (channels == 1)
        {
            return "M";
        }
        if (channels == 2)
        {
            if (midSide)
            {
                return channel == 0 ? "M" : "S";
            }
            return channel == 0 ? "L" : "R";
        }
        return std::to_string(channel + 1);
    }

    auto format_loudness(const std::optional<double>& value) -> std::string
    {
        if (!value.has_value())
        {
            return "--";
        }
        if (*value < s_silenceLoudness)
        {
            return "-inf";
        }
        return fmt::format("{:.1f}", *value);
    }

    ImPlotPoint vec2_to_plot_point(int idx, void* data) { return ImPlotPoint(((ImVec2*)data)[idx]); }

    struct waveform_view_settings
    {
        gluten::waveform_overlay overlay = gluten::waveform_overlay::loudness;
        bool renderMidSide               = false;
    };

    auto view_settings_for(const std::filesystem::path& filePath) -> waveform_view_settings&
    {
        static std::unordered_map<std::filesystem::path, waveform_view_settings> s_viewSettings;
        return s_viewSettings[filePath];
    }
}  // namespace

namespace gluten
{
    auto calc_tabular_text_width(ImFont* font, float fontSize, std::string_view text) -> float
    {
        const float digitAdvance = max_digit_advance(font, fontSize);
        float width              = 0.0f;
        for (const char& character : text)
        {
            width += std::isdigit(static_cast<unsigned char>(character)) ? digitAdvance : font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, &character, &character + 1).x;
        }
        return width;
    }

    auto draw_tabular_text(ImDrawList* drawList, ImFont* font, float fontSize, ImVec2 position, ImU32 color, std::string_view text) -> float
    {
        const float digitAdvance = max_digit_advance(font, fontSize);
        const float startX       = position.x;

        for (const char& character : text)
        {
            const float glyphWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, &character, &character + 1).x;

            if (std::isdigit(static_cast<unsigned char>(character)))
            {
                drawList->AddText(font, fontSize, ImVec2(position.x + (digitAdvance - glyphWidth) * 0.5f, position.y), color, &character, &character + 1);
                position.x += digitAdvance;
            }
            else
            {
                drawList->AddText(font, fontSize, position, color, &character, &character + 1);
                position.x += glyphWidth;
            }
        }

        return position.x - startX;
    }

    auto format_timecode(double seconds, bool showMilliseconds) -> std::string
    {
        const long long totalMilliseconds = std::llround(std::max(seconds, 0.0) * 1000.0);
        const long long minutes           = totalMilliseconds / 60000;
        const long long wholeSeconds      = (totalMilliseconds / 1000) % 60;
        const long long milliseconds      = totalMilliseconds % 1000;

        if (showMilliseconds)
        {
            return fmt::format("{:02}:{:02}.{:03}", minutes, wholeSeconds, milliseconds);
        }
        return fmt::format("{:02}:{:02}", minutes, wholeSeconds);
    }

    audio_element::audio_element(const std::filesystem::path& filePath)
        : element(anchor_preset::stretch_full), m_filePath(filePath)
    {
        m_waveformAndLoudnessLayout.set_layout_spacing(gluten::theme::space04);
        m_waveformAndLoudnessLayout.set_element_rounding(gluten::theme::radiusSm);

        m_audioBackground.set_element_background_color(gluten::theme::background);
        m_audioBackground.set_element_rounding(gluten::theme::radiusSm);
    }

    auto audio_element::get_position() const -> double
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            return audioSubsystem->get_sound_cursor_position(m_filePath);
        }
        return 0.0;
    }

    auto audio_element::get_duration() const -> double
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            return audioSubsystem->get_sound_length(m_filePath);
        }
        return 0.0;
    }

    auto audio_element::is_playing() const -> bool
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            return audioSubsystem->get_sound_is_playing(m_filePath);
        }
        return false;
    }

    auto audio_element::play() -> void
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            audioSubsystem->pause_all();
            audioSubsystem->play_sound(m_filePath);
        }
    }

    auto audio_element::pause() -> void
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            audioSubsystem->pause_sound(m_filePath);
        }
    }

    auto audio_element::seek(double position) -> void
    {
        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            audioSubsystem->set_sound_cursor_position(m_filePath, static_cast<float>(std::clamp(position, 0.0, m_fileDuration)));
        }
    }

    auto audio_element::set_overlay(waveform_overlay overlay) -> void { view_settings_for(m_filePath).overlay = overlay; }
    auto audio_element::get_overlay() const -> waveform_overlay { return view_settings_for(m_filePath).overlay; }
    auto audio_element::set_render_mid_side(bool renderMidSide) -> void { view_settings_for(m_filePath).renderMidSide = renderMidSide; }
    auto audio_element::get_render_mid_side() const -> bool { return view_settings_for(m_filePath).renderMidSide; }

    auto audio_element::get_channel_peaks_at(double position) const -> std::vector<float>
    {
        std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>();
        if (!audioSubsystem || m_fileDuration <= 0.0)
        {
            return {};
        }

        const auto& waveformLods = audioSubsystem->get_ui_waveform_lods(m_filePath, m_fileDuration);
        if (!waveformLods.has_data())
        {
            return {};
        }

        const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* cache = &waveformLods.m_cache.lowRes.get_cached_data();
        if (!cache->has_data())
        {
            cache = &waveformLods.m_cache.thumbnailRes.get_cached_data();
        }
        if (!cache->has_data())
        {
            return {};
        }

        const auto& channelFrames = cache->m_cache.lodWaveform.channelFrames;
        std::vector<float> peaks(channelFrames.size(), 0.0f);

        for (std::size_t channel = 0; channel < channelFrames.size(); ++channel)
        {
            const auto& frames = channelFrames[channel];
            if (frames.empty())
            {
                continue;
            }

            const int center = static_cast<int>(position / m_fileDuration * static_cast<double>(frames.size()));
            for (int index = std::max(center - 2, 0); index <= std::min(center + 2, static_cast<int>(frames.size()) - 1); ++index)
            {
                peaks[channel] = std::max({peaks[channel], std::abs(frames[index].min), std::abs(frames[index].max)});
            }
        }

        return peaks;
    }

    auto audio_element::render_element(const element_render_info& renderInfo) -> bool
    {
        ZoneScoped;

        m_filePosition = get_position();
        m_fileDuration = get_duration();

        m_waveformAndLoudnessLayout.render(renderInfo.elementBox);
        m_waveformAndLoudnessLayout.render_layout_element_percent_horizontal(&m_audioBackground, 1.0f);

        if (renderInfo.isVisible)
        {
            render_waveform();
        }

        return false;
    }

    auto audio_element::get_element_content_size(const ImVec2& parentSize) -> ImVec2 const
    {
        return ImVec2(parentSize.x, get_audio_height(parentSize.x));
    }

    auto audio_element::get_audio_height(float width) -> float
    {
        return width * 0.25f + s_rulerHeight + s_markerLaneHeight + s_statusBarHeight;
    }

    auto audio_element::render_waveform() -> void
    {
        ZoneScoped;

        std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>();
        if (!audioSubsystem)
        {
            return;
        }

        constexpr const char* s_plotLimitMinName = "Min";
        constexpr const char* s_plotLimitMaxName = "Max";

        constexpr float s_sampleResolutionThreshold = 0.1f;
        constexpr float s_highResolutionThreshold   = 1.0f;
        constexpr float s_mediumResolutionThreshold = 10.0f;
        constexpr float s_lowResolutionThreshold    = 100.0f;

        imgui::scoped_state_storage<float> plotLimitLeft(s_plotLimitMinName);
        imgui::scoped_state_storage<float> plotLimitRight(s_plotLimitMaxName);

        const auto& waveformLods = audioSubsystem->get_ui_waveform_lods(m_filePath, m_fileDuration);

        if (!waveformLods.has_data())
        {
            return;
        }

        const float plotTimeWidth = plotLimitRight - plotLimitLeft;

        bool sampleRes = false;

        const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* waveformCache  = &waveformLods.m_cache.thumbnailRes.get_cached_data();
        const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* peakCache      = &waveformLods.m_cache.thumbnailRes.get_cached_data();
        const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* thumbnailCache = &waveformLods.m_cache.thumbnailRes.get_cached_data();

        if (plotTimeWidth < s_lowResolutionThreshold && waveformLods.m_cache.lowRes.get_cached_data().has_data())
        {
            waveformCache = &waveformLods.m_cache.lowRes.get_cached_data();
        }

        if (plotTimeWidth < s_mediumResolutionThreshold && waveformLods.m_cache.medRes.get_cached_data().has_data())
        {
            waveformCache = &waveformLods.m_cache.medRes.get_cached_data();
        }

        if (plotTimeWidth < s_highResolutionThreshold && waveformLods.m_cache.highRes.get_cached_data().has_data())
        {
            waveformCache = &waveformLods.m_cache.highRes.get_cached_data();

            if (waveformLods.m_cache.lowRes.get_cached_data().has_data())
            {
                peakCache = &waveformLods.m_cache.lowRes.get_cached_data();
            }
        }

        if (plotTimeWidth < s_sampleResolutionThreshold && waveformLods.m_cache.sampleRes.get_cached_data().has_data())
        {
            waveformCache = &waveformLods.m_cache.sampleRes.get_cached_data();
            sampleRes     = true;

            if (waveformLods.m_cache.medRes.get_cached_data().has_data())
            {
                peakCache = &waveformLods.m_cache.medRes.get_cached_data();
            }
        }

        if (!waveformCache || !waveformCache->has_data() || !peakCache)
        {
            return;
        }

        const std::size_t channels = waveformCache->m_cache.lodWaveform.channelFrames.size();
        m_channelCount             = channels;

        if (channels == 0)
        {
            return;
        }

        const auto& sampleResCache = waveformLods.m_cache.sampleRes.get_cached_data();
        if (sampleResCache.has_data())
        {
            const loudness_lufs_global& lufs = sampleResCache.m_cache.lodWaveform.lufs;
            m_loudnessStats.integrated       = lufs.integrated;
            m_loudnessStats.range            = lufs.range;
            m_loudnessStats.truePeak         = lufs.truePeak;
        }

        const waveform_overlay overlay = get_overlay();
        const bool midSideRequested    = get_render_mid_side();

        if (thumbnailCache->has_data() && thumbnailCache->m_cache.lodWaveform.globalFramesCache.get_cached_data().has_data())
        {
            double shorttermMax = -200.0;
            for (const frame_data& frame : thumbnailCache->m_cache.lodWaveform.globalFramesCache.get_cached_data().m_cache)
            {
                shorttermMax = std::max(shorttermMax, frame.lufs.shortterm);
            }
            m_loudnessStats.shorttermMax = shorttermMax;
        }

        if (thumbnailCache->has_data())
        {
            float samplePeak = 0.0f;
            for (const std::vector<channel_frame>& frames : thumbnailCache->m_cache.lodWaveform.channelFrames)
            {
                for (const channel_frame& frame : frames)
                {
                    samplePeak = std::max({samplePeak, std::abs(frame.min), std::abs(frame.max)});
                }
            }
            m_loudnessStats.samplePeak = samplePeak > 0.0f ? 20.0 * std::log10(samplePeak) : -200.0;
        }

        const ImRect area = m_audioBackground.get_element_rect();
        const ImRect header(area.Min, ImVec2(area.Min.x + s_trackHeaderWidth, area.Max.y - s_statusBarHeight));
        const ImRect ruler(ImVec2(header.Max.x, area.Min.y), ImVec2(area.Max.x, area.Min.y + s_rulerHeight));
        const ImRect markerLane(ImVec2(header.Max.x, ruler.Max.y), ImVec2(area.Max.x, ruler.Max.y + s_markerLaneHeight));
        const ImRect lanes(ImVec2(header.Max.x, markerLane.Max.y), ImVec2(area.Max.x, header.Max.y));
        const ImRect statusBar(ImVec2(area.Min.x, lanes.Max.y), area.Max);

        if (lanes.GetWidth() <= 1.0f || lanes.GetHeight() <= 1.0f)
        {
            return;
        }

        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        ImFont* const font         = gluten::app::get()->get_font(fonts::regular);

        const ImU32 chromeColor      = ImGui::GetColorU32(gluten::theme::layer02);
        const ImU32 dividerColor     = ImGui::GetColorU32(gluten::theme::background);
        const ImU32 markerLaneColor  = ImGui::GetColorU32(ImLerp(gluten::theme::background, gluten::theme::layer01, 0.5f));
        const ImU32 labelColor       = ImGui::GetColorU32(gluten::theme::textSecondary);
        const ImU32 faintLabelColor  = ImGui::GetColorU32(gluten::theme::textHelper);
        const ImU32 majorTickColor   = ImGui::GetColorU32(gluten::theme::textHelper);
        const ImU32 minorTickColor   = ImGui::GetColorU32(gluten::theme::borderStrong01);
        const ImU32 gridColor        = ImGui::GetColorU32(gluten::theme::adjust_alpha(gluten::theme::textPrimary, 0.04f));
        const ImU32 centerLineColor  = ImGui::GetColorU32(gluten::theme::borderSubtle00);
        const ImU32 unplayedDimColor = ImGui::GetColorU32(gluten::theme::adjust_alpha(gluten::theme::background, 0.35f));
        const ImU32 clipColor        = ImGui::GetColorU32(gluten::theme::supportError);
        const ImU32 loudnessColor    = ImGui::GetColorU32(gluten::theme::supportWarning);
        const ImU32 playheadColor    = ImGui::GetColorU32(gluten::theme::textPrimary);

        drawList->AddRectFilled(header.Min, header.Max, chromeColor, gluten::theme::radiusSm, ImDrawFlags_RoundCornersTopLeft);
        drawList->AddRectFilled(ruler.Min, ruler.Max, chromeColor, gluten::theme::radiusSm, ImDrawFlags_RoundCornersTopRight);
        drawList->AddRectFilled(markerLane.Min, markerLane.Max, markerLaneColor);

        ImRect timeArea   = lanes;  //< Where ImPlot actually maps time; the ruler and playhead must use the same span
        double visibleMin = plotLimitLeft;
        double visibleMax = plotLimitRight;
        bool plotHovered  = false;
        float hoveredX    = 0.0f;
        std::vector<float> laneCentersPixels(channels, 0.0f);
        std::vector<float> laneTopsPixels(channels, 0.0f);

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0.0f, 0.0f));
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
        ImPlot::PushStyleColor(ImPlotCol_PlotBg, gluten::theme::background);

        ImGui::SetCursorScreenPos(lanes.Min);

        if (ImPlot::BeginPlot(fmt::format("##Waveform{}", m_filePath.filename().string()).c_str(), lanes.GetSize(),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame))
        {
            ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, m_fileDuration);
            ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, 0.0, m_fileDuration);

            const float minYValue = lane_center(channels - 1) - s_laneHalfHeight;

            ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock);
            ImPlot::SetupAxisLimits(ImAxis_Y1, minYValue, s_laneHalfHeight, ImPlotCond_Always);

            // Each extra Y axis on the same side makes ImPlot pad the plot area, so only the overlay's axis is
            // enabled and it sits opposite Y1.
            if (overlay == waveform_overlay::bands)
            {
                ImPlot::SetupAxis(ImAxis_Y2, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock | ImPlotAxisFlags_Opposite);
                ImPlot::SetupAxisLimits(ImAxis_Y2, s_bandsAxisMin, 0.0, ImPlotCond_Always);
            }
            else if (overlay == waveform_overlay::loudness)
            {
                ImPlot::SetupAxis(ImAxis_Y3, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Lock | ImPlotAxisFlags_Opposite);
                ImPlot::SetupAxisLimits(ImAxis_Y3, s_lufsAxisMin, s_lufsAxisMax, ImPlotCond_Always);
            }

            ImPlot::SetupFinish();

            timeArea = ImRect(ImPlot::GetPlotPos(), ImVec2(ImPlot::GetPlotPos().x + ImPlot::GetPlotSize().x, ImPlot::GetPlotPos().y + ImPlot::GetPlotSize().y));

            visibleMin = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1).X.Min;
            visibleMax = ImPlot::GetPlotLimits(ImAxis_X1, ImAxis_Y1).X.Max;

            ImDrawList* const plotDrawList = ImPlot::GetPlotDrawList();
            const double pixelsPerSecond   = timeArea.GetWidth() / std::max(visibleMax - visibleMin, 1e-6);

            ImPlot::PushPlotClipRect();

            {
                const ruler_step step = pick_ruler_step(pixelsPerSecond, 90.0f);
                for (long long index = static_cast<long long>(std::floor(visibleMin / step.major)); index <= static_cast<long long>(std::ceil(visibleMax / step.major)); ++index)
                {
                    const float x = std::floor(ImPlot::PlotToPixels(index * step.major, 0.0, ImAxis_X1, ImAxis_Y1).x) + 0.5f;
                    plotDrawList->AddLine(ImVec2(x, lanes.Min.y), ImVec2(x, lanes.Max.y), gridColor);
                }
            }

            for (std::size_t channel = 0; channel < channels; ++channel)
            {
                const ImVec2 center        = ImPlot::PlotToPixels(0.0, lane_center(channel), ImAxis_X1, ImAxis_Y1);
                const ImVec2 top           = ImPlot::PlotToPixels(0.0, lane_center(channel) + s_laneHalfHeight, ImAxis_X1, ImAxis_Y1);
                laneCentersPixels[channel] = std::floor(center.y) + 0.5f;
                laneTopsPixels[channel]    = top.y;

                plotDrawList->AddLine(ImVec2(lanes.Min.x, laneCentersPixels[channel]), ImVec2(lanes.Max.x, laneCentersPixels[channel]), centerLineColor);
            }

            ImPlot::PopPlotClipRect();

            using fill_channel_points_callback_type = std::function<void(int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)>;

            const auto get_render_points_at_cache_resolution = [fileDuration = m_fileDuration, plotLimitLeft, plotLimitRight](const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* cache) -> int
            {
                if (cache && cache->has_data())
                {
                    const int numWaveformPoints                      = (int)(fileDuration * cache->m_cache.resolution);
                    const int plotLimitLeftAsPointIndexAtCurrentRes  = (plotLimitLeft / fileDuration) * numWaveformPoints;
                    const int plotLimitRightAsPointIndexAtCurrentRes = (std::clamp((plotLimitRight + 1.0f), 0.0f, (float)fileDuration) / fileDuration) * numWaveformPoints;

                    return std::max(plotLimitRightAsPointIndexAtCurrentRes - plotLimitLeftAsPointIndexAtCurrentRes, 0);
                }
                else
                {
                    return 0;
                }
            };

            const auto fill_channel_points = [fileDuration = m_fileDuration, &plotLimitLeft, &plotLimitRight, &channels](const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* cache, int channel, const fill_channel_points_callback_type& callback) -> void
            {
                if (!cache || !cache->has_data())
                {
                    return;
                }

                const int numWaveformPoints                      = (int)(fileDuration * cache->m_cache.resolution);
                const int plotLimitLeftAsPointIndexAtCurrentRes  = (plotLimitLeft / fileDuration) * numWaveformPoints;
                const int plotLimitRightAsPointIndexAtCurrentRes = (std::clamp((plotLimitRight + 1.0f), 0.0f, (float)fileDuration) / fileDuration) * numWaveformPoints;

                const int numberOfPointsAtCurrentResolution = plotLimitRightAsPointIndexAtCurrentRes - plotLimitLeftAsPointIndexAtCurrentRes;

                int point = plotLimitLeftAsPointIndexAtCurrentRes;
                int index = 0;

                for (; point < numWaveformPoints && point <= plotLimitRightAsPointIndexAtCurrentRes && index < numberOfPointsAtCurrentResolution; ++point, ++index)
                {
                    const int clampedIndex = std::clamp(point, 0, numWaveformPoints - 1);

                    const double xPosition = (double)point / (numWaveformPoints)*fileDuration;

                    const auto& globalData  = cache->m_cache.lodWaveform.globalFramesCache;
                    const auto& channelData = cache->m_cache.lodWaveform.channelFrames[channel][clampedIndex];
                    if (channels == 2)
                    {
                        const auto& stereoData = cache->m_cache.lodWaveform.stereoFrames[clampedIndex];
                        callback(index, xPosition, point, channelData, stereoData, globalData);
                    }
                    else
                    {
                        const gluten::stereo_data fakeStereoData;
                        callback(index, xPosition, point, channelData, fakeStereoData, globalData);
                    }
                }
            };

            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);

            static ImPlotSpec peakSpec;
            static ImPlotSpec rmsSpec;
            peakSpec.LineColor = gluten::theme::interactive;
            peakSpec.FillColor = gluten::theme::interactive;
            rmsSpec.FillColor  = gluten::theme::hex_to_imgui_imvec4(0xa6c8ff);

            const bool renderMidSide = midSideRequested && channels == 2 && !sampleRes;

            std::vector<std::pair<float, bool>> clipPoints;  //< x position, true when clipping at the bottom

            for (int channel = 0; channel < static_cast<int>(channels); ++channel)
            {
                const int numberOfPointsAtCurrentResolution = get_render_points_at_cache_resolution(waveformCache);
                const float center                          = lane_center(channel);

                std::vector<ImVec2> upperPoints(sampleRes ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                std::vector<ImVec2> lowerPoints(sampleRes ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                std::vector<ImVec2> rmsUpperPoints(sampleRes || renderMidSide ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                std::vector<ImVec2> rmsLowerPoints(sampleRes || renderMidSide ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                std::vector<ImVec2> samplePoints(sampleRes ? numberOfPointsAtCurrentResolution : 0, ImVec2());

                clipPoints.clear();

                fill_channel_points(waveformCache, channel, [&](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                                    {
                    if (sampleRes)
                    {
                        samplePoints[index] = ImVec2(xPosition, channelFrame.sample + center);
                    }
                    else if (renderMidSide)
                    {
                        upperPoints[index] = ImVec2(xPosition, (channel == 0 ? stereoData.midMax : stereoData.sideMax) + center);
                        lowerPoints[index] = ImVec2(xPosition, (channel == 0 ? stereoData.midMin : stereoData.sideMin) + center);
                    }
                    else
                    {
                        upperPoints[index]    = ImVec2(xPosition, channelFrame.max + center);
                        lowerPoints[index]    = ImVec2(xPosition, channelFrame.min + center);
                        rmsUpperPoints[index] = ImVec2(xPosition, center + channelFrame.rms);
                        rmsLowerPoints[index] = ImVec2(xPosition, center - channelFrame.rms);

                        if (channelFrame.max >= s_clipThreshold)
                        {
                            clipPoints.emplace_back(static_cast<float>(xPosition), false);
                        }
                        if (channelFrame.min <= -s_clipThreshold)
                        {
                            clipPoints.emplace_back(static_cast<float>(xPosition), true);
                        }
                    } });

                if (sampleRes)
                {
                    ImPlot::PlotStairsG("##Waveform", vec2_to_plot_point, samplePoints.data(), static_cast<int>(samplePoints.size()), peakSpec);
                }
                else
                {
                    ImPlot::PlotShadedG("##Waveform", vec2_to_plot_point, upperPoints.data(), vec2_to_plot_point, lowerPoints.data(), static_cast<int>(lowerPoints.size()), peakSpec);
                    ImPlot::PlotLineG("##Waveform", vec2_to_plot_point, upperPoints.data(), static_cast<int>(upperPoints.size()), peakSpec);
                    ImPlot::PlotLineG("##Waveform", vec2_to_plot_point, lowerPoints.data(), static_cast<int>(lowerPoints.size()), peakSpec);

                    if (!rmsUpperPoints.empty())
                    {
                        ImPlot::PlotShadedG("##WaveformRms", vec2_to_plot_point, rmsUpperPoints.data(), vec2_to_plot_point, rmsLowerPoints.data(), static_cast<int>(rmsLowerPoints.size()), rmsSpec);
                    }
                }

                if (!clipPoints.empty())
                {
                    ImPlot::PushPlotClipRect();
                    const float laneTop    = ImPlot::PlotToPixels(0.0, center + s_laneHalfHeight, ImAxis_X1, ImAxis_Y1).y;
                    const float laneBottom = ImPlot::PlotToPixels(0.0, center - s_laneHalfHeight, ImAxis_X1, ImAxis_Y1).y;
                    for (const auto& [time, bottom] : clipPoints)
                    {
                        const float x = ImPlot::PlotToPixels(time, 0.0, ImAxis_X1, ImAxis_Y1).x;
                        const float y = bottom ? laneBottom - 3.0f : laneTop;
                        ImPlot::GetPlotDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + 2.0f, y + 3.0f), clipColor);
                    }
                    ImPlot::PopPlotClipRect();
                }
            }

            {
                ImPlot::PushPlotClipRect();
                const float playheadX = ImPlot::PlotToPixels(m_filePosition, 0.0, ImAxis_X1, ImAxis_Y1).x;
                if (playheadX < lanes.Max.x)
                {
                    plotDrawList->AddRectFilled(ImVec2(std::max(playheadX, lanes.Min.x), lanes.Min.y), lanes.Max, unplayedDimColor);
                }

                for (std::size_t channel = 0; channel + 1 < channels; ++channel)
                {
                    const float y = std::floor(ImPlot::PlotToPixels(0.0, lane_center(channel) - s_laneHalfHeight, ImAxis_X1, ImAxis_Y1).y) + 0.5f;
                    plotDrawList->AddLine(ImVec2(lanes.Min.x, y), ImVec2(lanes.Max.x, y), dividerColor);
                }

                if (lanes.GetHeight() / static_cast<float>(channels) > 60.0f && !renderMidSide)
                {
                    const float labelSize = gluten::theme::textSizeCaption;
                    for (std::size_t channel = 0; channel < channels; ++channel)
                    {
                        const float fullScaleY = ImPlot::PlotToPixels(0.0, lane_center(channel) + 1.0f, ImAxis_X1, ImAxis_Y1).y;
                        const float minusSixY  = ImPlot::PlotToPixels(0.0, lane_center(channel) + 0.5f, ImAxis_X1, ImAxis_Y1).y;
                        const float zeroWidth  = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, "0").x;
                        const float sixWidth   = font->CalcTextSizeA(labelSize, FLT_MAX, 0.0f, "-6").x;
                        plotDrawList->AddText(font, labelSize, ImVec2(lanes.Max.x - zeroWidth - 4.0f, fullScaleY), faintLabelColor, "0");
                        plotDrawList->AddText(font, labelSize, ImVec2(lanes.Max.x - sixWidth - 4.0f, minusSixY - labelSize * 0.5f), faintLabelColor, "-6");
                    }
                }
                ImPlot::PopPlotClipRect();
            }

            if (overlay == waveform_overlay::bands)
            {
                const int numberOfPeakPoints = get_render_points_at_cache_resolution(peakCache);

                std::vector<ImVec2> lowDecibelPoints(numberOfPeakPoints, ImVec2());
                std::vector<ImVec2> midDecibelPoints(numberOfPeakPoints, ImVec2());
                std::vector<ImVec2> highDecibelPoints(numberOfPeakPoints, ImVec2());

                fill_channel_points(peakCache, 0, [&](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                                    {
                    if (globalFrameCache.get_cached_data().has_data())
                    {
                        const auto& globalFrameData = globalFrameCache.get_cached_data().m_cache[point];

                        lowDecibelPoints[index]  = ImVec2(xPosition, std::max(ma_volume_linear_to_db(globalFrameData.lowAverage), s_bandsAxisMin + 1.0f));
                        midDecibelPoints[index]  = ImVec2(xPosition, std::max(ma_volume_linear_to_db(globalFrameData.midAverage), s_bandsAxisMin + 1.0f));
                        highDecibelPoints[index] = ImVec2(xPosition, std::max(ma_volume_linear_to_db(globalFrameData.highAverage), s_bandsAxisMin + 1.0f));
                    } });

                static ImPlotSpec lowSpec;
                static ImPlotSpec midSpec;
                static ImPlotSpec highSpec;
                lowSpec.LineColor  = gluten::theme::supportError;
                midSpec.LineColor  = gluten::theme::supportWarning;
                highSpec.LineColor = gluten::theme::supportSuccess;
                lowSpec.LineWeight = midSpec.LineWeight = highSpec.LineWeight = 1.5f;

                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);
                ImPlot::PlotLineG("##Lows", vec2_to_plot_point, lowDecibelPoints.data(), numberOfPeakPoints, lowSpec);
                ImPlot::PlotLineG("##Mids", vec2_to_plot_point, midDecibelPoints.data(), numberOfPeakPoints, midSpec);
                ImPlot::PlotLineG("##Highs", vec2_to_plot_point, highDecibelPoints.data(), numberOfPeakPoints, highSpec);
            }
            else if (overlay == waveform_overlay::loudness)
            {
                const int numberOfLufsPoints = get_render_points_at_cache_resolution(thumbnailCache);

                std::vector<ImVec2> shorttermPoints(numberOfLufsPoints, ImVec2());

                fill_channel_points(thumbnailCache, 0, [&](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                                    {
                    if (globalFrameCache.get_cached_data().has_data())
                    {
                        const auto& globalFrameData = globalFrameCache.get_cached_data().m_cache[point];
                        shorttermPoints[index]      = ImVec2(xPosition, std::max(static_cast<float>(globalFrameData.lufs.shortterm), s_lufsAxisMin));
                    } });

                static ImPlotSpec shorttermSpec;
                shorttermSpec.LineColor  = gluten::theme::supportWarning;
                shorttermSpec.LineWeight = 1.5f;

                ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);
                ImPlot::PlotLineG("##Shortterm", vec2_to_plot_point, shorttermPoints.data(), numberOfLufsPoints, shorttermSpec);

                if (m_loudnessStats.integrated.has_value() && *m_loudnessStats.integrated > s_lufsAxisMin)
                {
                    ImPlot::PushPlotClipRect();
                    const float y         = std::floor(ImPlot::PlotToPixels(0.0, *m_loudnessStats.integrated, ImAxis_X1, ImAxis_Y3).y) + 0.5f;
                    const ImU32 dashColor = ImGui::GetColorU32(gluten::theme::adjust_alpha(gluten::theme::supportWarning, 0.6f));
                    for (float x = lanes.Min.x; x < lanes.Max.x; x += 7.0f)
                    {
                        plotDrawList->AddLine(ImVec2(x, y), ImVec2(std::min(x + 4.0f, lanes.Max.x), y), dashColor);
                    }

                    const std::string label = fmt::format("{:.1f} LUFS-I", *m_loudnessStats.integrated);
                    const float labelSize   = gluten::theme::textSizeCaption;
                    const float labelWidth  = calc_tabular_text_width(font, labelSize, label);
                    draw_tabular_text(plotDrawList, font, labelSize, ImVec2(lanes.Max.x - labelWidth - 22.0f, y - labelSize - 2.0f), loudnessColor, label);
                    ImPlot::PopPlotClipRect();
                }
            }

            ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);

            render_waveform_overlay(markerLane, plotTimeWidth);

            if (ImPlot::IsPlotHovered())
            {
                plotHovered = true;
                hoveredX    = ImGui::GetMousePos().x;

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    seek(ImPlot::GetPlotMousePos().x);
                }

                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                {
                    if (is_playing())
                    {
                        pause();
                    }
                    else
                    {
                        play();
                    }
                }
            }

            plotLimitLeft  = ImPlot::GetPlotLimits().X.Min;
            plotLimitRight = ImPlot::GetPlotLimits().X.Max;

            ImPlot::EndPlot();
        }

        ImPlot::PopStyleColor();
        ImPlot::PopStyleVar(2);

        const auto time_to_pixel = [&](double time) -> float
        {
            return timeArea.Min.x + static_cast<float>((time - visibleMin) / std::max(visibleMax - visibleMin, 1e-6)) * timeArea.GetWidth();
        };
        const auto pixel_to_time = [&](float x) -> double
        {
            return visibleMin + (x - timeArea.Min.x) / timeArea.GetWidth() * (visibleMax - visibleMin);
        };

        drawList->PushClipRect(ruler.Min, ruler.Max, true);
        {
            const float labelSize        = gluten::theme::textSizeCaption;
            const double pixelsPerSecond = timeArea.GetWidth() / std::max(visibleMax - visibleMin, 1e-6);
            const bool showMilliseconds  = pick_ruler_step(pixelsPerSecond, calc_tabular_text_width(font, labelSize, "00:00") + 16.0f).major < 1.0;
            const float labelWidth       = calc_tabular_text_width(font, labelSize, showMilliseconds ? "00:00.000" : "00:00") + 16.0f;
            const ruler_step step        = pick_ruler_step(pixelsPerSecond, labelWidth);
            const double minorStep       = step.major / step.minorDivisions;

            const long long first = static_cast<long long>(std::floor(visibleMin / minorStep));
            const long long last  = static_cast<long long>(std::ceil(visibleMax / minorStep));

            for (long long index = first; index <= last && index - first < 4000; ++index)
            {
                const double time  = index * minorStep;
                const float x      = std::floor(time_to_pixel(time)) + 0.5f;
                const bool isMajor = index % step.minorDivisions == 0;

                drawList->AddLine(ImVec2(x, ruler.Max.y - (isMajor ? 8.0f : 3.0f)), ImVec2(x, ruler.Max.y), isMajor ? majorTickColor : minorTickColor);

                if (isMajor)
                {
                    draw_tabular_text(drawList, font, labelSize, ImVec2(x + 3.0f, ruler.Min.y + 2.0f), labelColor, format_timecode(time, showMilliseconds));
                }
            }
        }
        drawList->PopClipRect();

        drawList->AddLine(ImVec2(ruler.Min.x, ruler.Max.y - 0.5f), ImVec2(ruler.Max.x, ruler.Max.y - 0.5f), dividerColor);
        drawList->AddLine(ImVec2(markerLane.Min.x, markerLane.Max.y - 0.5f), ImVec2(markerLane.Max.x, markerLane.Max.y - 0.5f), dividerColor);
        drawList->AddLine(ImVec2(header.Max.x - 0.5f, header.Min.y), ImVec2(header.Max.x - 0.5f, header.Max.y), dividerColor);

        for (std::size_t channel = 0; channel < channels; ++channel)
        {
            const std::string label = channel_label(channel, channels, midSideRequested && channels == 2);
            const float labelWidth  = font->CalcTextSizeA(gluten::theme::textSizeHelper, FLT_MAX, 0.0f, label.c_str()).x;
            drawList->AddText(font, gluten::theme::textSizeHelper, ImVec2(header.GetCenter().x - labelWidth * 0.5f, laneCentersPixels[channel] - gluten::theme::textSizeHelper * 0.5f), labelColor, label.c_str());

            if (channel + 1 < channels)
            {
                const float y = std::floor((laneCentersPixels[channel] + laneCentersPixels[channel + 1]) * 0.5f) + 0.5f;
                drawList->AddLine(ImVec2(header.Min.x, y), ImVec2(header.Max.x, y), dividerColor);
            }
        }
        drawList->AddLine(ImVec2(header.Min.x, ruler.Max.y - 0.5f), ImVec2(header.Max.x, ruler.Max.y - 0.5f), dividerColor);
        drawList->AddLine(ImVec2(header.Min.x, markerLane.Max.y - 0.5f), ImVec2(header.Max.x, markerLane.Max.y - 0.5f), dividerColor);

        ImGui::SetCursorScreenPos(ruler.Min);
        ImGui::InvisibleButton(fmt::format("##Ruler{}", m_filePath.filename().string()).c_str(), ruler.GetSize());
        const bool isScrubbing = ImGui::IsItemActive();
        if (isScrubbing)
        {
            seek(pixel_to_time(std::clamp(ImGui::GetMousePos().x, timeArea.Min.x, timeArea.Max.x)));
        }
        if (ImGui::IsItemHovered())
        {
            plotHovered = true;
            hoveredX    = ImGui::GetMousePos().x;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        drawList->PushClipRect(ImVec2(lanes.Min.x - 8.0f, ruler.Min.y), ImVec2(lanes.Max.x + 8.0f, lanes.Max.y), true);

        if (plotHovered && !isScrubbing)
        {
            const float x = std::floor(hoveredX) + 0.5f;
            drawList->AddLine(ImVec2(x, markerLane.Min.y), ImVec2(x, lanes.Max.y), ImGui::GetColorU32(gluten::theme::adjust_alpha(gluten::theme::textPrimary, 0.25f)));

            const std::string hoverTime = format_timecode(pixel_to_time(hoveredX));
            const float labelSize       = gluten::theme::textSizeCaption;
            const float width           = calc_tabular_text_width(font, labelSize, hoverTime) + 8.0f;
            constexpr float s_cursorGap = 6.0f;
            const bool flipLeft         = x + s_cursorGap + width > ruler.Max.x;
            const float left            = flipLeft ? x - s_cursorGap - width : x + s_cursorGap;
            drawList->AddRectFilled(ImVec2(left, ruler.Min.y + 1.0f), ImVec2(left + width, ruler.Max.y - 1.0f), ImGui::GetColorU32(gluten::theme::layer03), 2.0f);
            draw_tabular_text(drawList, font, labelSize, ImVec2(left + 4.0f, ruler.Min.y + 2.0f), ImGui::GetColorU32(gluten::theme::textPrimary), hoverTime);
        }

        const float playheadX = std::floor(time_to_pixel(m_filePosition)) + 0.5f;
        if (playheadX >= lanes.Min.x - 1.0f && playheadX <= lanes.Max.x + 1.0f)
        {
            drawList->AddLine(ImVec2(playheadX, ruler.Min.y), ImVec2(playheadX, lanes.Max.y), playheadColor);

            const float handleHalfWidth = 5.5f;
            const ImVec2 handle[]       = {ImVec2(playheadX - handleHalfWidth, ruler.Min.y),
                                           ImVec2(playheadX + handleHalfWidth, ruler.Min.y),
                                           ImVec2(playheadX + handleHalfWidth, ruler.Min.y + 7.0f),
                                           ImVec2(playheadX, ruler.Min.y + 12.0f),
                                           ImVec2(playheadX - handleHalfWidth, ruler.Min.y + 7.0f)};
            drawList->AddConvexPolyFilled(handle, IM_ARRAYSIZE(handle), playheadColor);
        }

        if (thumbnailCache->has_data() && thumbnailCache->m_cache.lodWaveform.globalFramesCache.get_cached_data().has_data())
        {
            const std::vector<frame_data>& frames = thumbnailCache->m_cache.lodWaveform.globalFramesCache.get_cached_data().m_cache;
            const double secondsPerFrame          = m_fileDuration / static_cast<double>(std::max<std::size_t>(frames.size(), 1));
            const ImU32 overColor                 = ImGui::GetColorU32(gluten::theme::supportError);

            std::size_t index = 0;
            while (index < frames.size())
            {
                if (frames[index].truePeak <= 1.0f)
                {
                    ++index;
                    continue;
                }

                const std::size_t spanStart = index;
                float spanPeak              = 0.0f;
                while (index < frames.size() && frames[index].truePeak > 1.0f)
                {
                    spanPeak = std::max(spanPeak, frames[index].truePeak);
                    ++index;
                }

                const float left  = std::floor(time_to_pixel(spanStart * secondsPerFrame));
                const float right = std::max(std::floor(time_to_pixel(index * secondsPerFrame)), left + 2.0f);
                const ImRect over(ImVec2(left, lanes.Min.y), ImVec2(right, lanes.Min.y + 3.0f));
                drawList->AddRectFilled(over.Min, over.Max, overColor);

                if (ImGui::IsMouseHoveringRect(ImVec2(over.Min.x - 2.0f, over.Min.y - 2.0f), ImVec2(over.Max.x + 2.0f, over.Max.y + 4.0f)))
                {
                    ImGui::SetTooltip("Inter-sample over: true peak %+.1f dBTP\n", 20.0 * std::log10(spanPeak));
                }
            }
        }

        drawList->PopClipRect();

        render_status_bar(statusBar);
    }

    auto audio_element::render_status_bar(const ImRect& statusBar) -> void
    {
        ImDrawList* const drawList = ImGui::GetWindowDrawList();
        ImFont* const font         = gluten::app::get()->get_font(fonts::regular);

        const float textSize     = gluten::theme::textSizeHelper;
        const float textY        = statusBar.Min.y + (statusBar.GetHeight() - textSize) * 0.5f - 1.0f;
        const float cellPadding  = gluten::theme::space08 + 2.0f;
        const ImU32 labelColor   = ImGui::GetColorU32(gluten::theme::textHelper);
        const ImU32 valueColor   = ImGui::GetColorU32(gluten::theme::textSecondary);
        const ImU32 dividerColor = ImGui::GetColorU32(gluten::theme::background);

        drawList->AddRectFilled(statusBar.Min, statusBar.Max, ImGui::GetColorU32(gluten::theme::layer02), gluten::theme::radiusSm, ImDrawFlags_RoundCornersBottom);
        drawList->AddLine(ImVec2(statusBar.Min.x, statusBar.Min.y + 0.5f), ImVec2(statusBar.Max.x, statusBar.Min.y + 0.5f), dividerColor);

        drawList->PushClipRect(statusBar.Min, statusBar.Max, true);

        float x = statusBar.Min.x;

        const auto draw_cell = [&](const char* label, const std::string& value, const char* unit, ImU32 color)
        {
            x += cellPadding;
            drawList->AddText(font, textSize, ImVec2(x, textY), labelColor, label);
            x += font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, label).x + 5.0f;
            x += draw_tabular_text(drawList, font, textSize, ImVec2(x, textY), color, value);
            if (unit)
            {
                x += 4.0f;
                drawList->AddText(font, textSize, ImVec2(x, textY), labelColor, unit);
                x += font->CalcTextSizeA(textSize, FLT_MAX, 0.0f, unit).x;
            }
            x += cellPadding;
            drawList->AddLine(ImVec2(std::floor(x) + 0.5f, statusBar.Min.y + 1.0f), ImVec2(std::floor(x) + 0.5f, statusBar.Max.y), dividerColor);
        };

        constexpr double s_truePeakWarning = -1.0;
        const bool truePeakClipped         = m_loudnessStats.truePeak.has_value() && *m_loudnessStats.truePeak >= 0.0;
        const bool truePeakHot             = m_loudnessStats.truePeak.has_value() && *m_loudnessStats.truePeak > s_truePeakWarning;

        draw_cell("LUFS-I", format_loudness(m_loudnessStats.integrated), nullptr, ImGui::GetColorU32(gluten::theme::supportWarning));
        draw_cell("Sample peak", format_loudness(m_loudnessStats.samplePeak), "dBFS", valueColor);
        draw_cell("True peak", format_loudness(m_loudnessStats.truePeak), "dBTP", truePeakClipped ? ImGui::GetColorU32(gluten::theme::supportError) : truePeakHot ? ImGui::GetColorU32(gluten::theme::supportWarning) : valueColor);
        draw_cell("LRA", format_loudness(m_loudnessStats.range), "LU", valueColor);
        draw_cell("Short-term max", format_loudness(m_loudnessStats.shorttermMax), "LUFS", valueColor);

        const std::string info = fmt::format("{} ch  {}", m_channelCount, format_timecode(m_fileDuration));
        const float infoWidth  = calc_tabular_text_width(font, textSize, info);
        if (statusBar.Max.x - infoWidth - cellPadding > x)
        {
            draw_tabular_text(drawList, font, textSize, ImVec2(statusBar.Max.x - infoWidth - cellPadding, textY), labelColor, info);
        }

        drawList->PopClipRect();
    }
}  // namespace gluten
