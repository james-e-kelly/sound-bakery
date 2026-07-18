#include "audio_element.h"

#include "gluten/app/app.h"
#include "gluten/subsystems/audio_subsystem.h"
#include "gluten/theme/theme.h"

#include "implot.h"
#include "implot_internal.h"

namespace gluten
{
    audio_element::audio_element(const std::filesystem::path& filePath)
        : element(anchor_preset::stretch_full), m_filePath(filePath)
    {
        m_waveformAndLoudnessLayout.set_layout_spacing(gluten::theme::padding);
        m_waveformAndLoudnessLayout.set_element_rounding(gluten::theme::rounding);

        m_audioBackground.set_element_background_color(gluten::theme::layer02);
        m_audioBackground.set_element_rounding(gluten::theme::rounding);
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
            audioSubsystem->set_sound_cursor_position(m_filePath, static_cast<float>(position));
        }
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
        return width * 0.33f;
    }

    static int time_formatter(double value, char* buff, int size, void* data)
    {
        const double minute = std::floor(value / 60.0);
        const double second = std::floor(std::fmod(value, 60.0));
        const double millisecond = std::fmod(value, 1.0);

        if (millisecond > 0.0)
        {
            return snprintf(buff, size, "%02g:%02g.%2g", minute, second, std::floor(millisecond * 1000.0));
        }
        else
        {
            return snprintf(buff, size, "%02g:%02g", minute, second);
        }
    }

    static ImPlotPoint vec2_to_plot_point(int idx, void* data)
    {
        return ImPlotPoint(((ImVec2*)data)[idx]);
    }

    static ImPlotPoint vec2_to_min_db_plot_point(int idx, void* data)
    {
        const ImVec2 vec2 = ((ImVec2*)data)[idx];
        return ImPlotPoint(vec2.x, -96.0);
    }

    auto audio_element::render_waveform() -> void
    {
        ZoneScoped;

        if (std::shared_ptr<gluten::audio_subsystem> audioSubsystem = gluten::app::get()->get_subsystem_by_class<gluten::audio_subsystem>())
        {
            if (ImDrawList* const drawList = ImGui::GetWindowDrawList())
            {
                constexpr const char* s_plotLimitMinName    = "Min";
                constexpr const char* s_plotLimitMaxName    = "Max";
                constexpr const char* s_minimumDecibelName  = "Min dB";
                constexpr const char* s_renderMidSideName   = "Mid/Side";
                constexpr const char* s_renderLowsName      = "Lows";
                constexpr const char* s_renderMidsName      = "Mids";
                constexpr const char* s_renderHighsName     = "Highs";
                constexpr const char* s_overlayModeName     = "Overlay Mode";
                constexpr const char* s_renderMomentaryName = "Momentary";
                constexpr const char* s_renderShorttermName = "Shortterm";

                constexpr const char* s_waveformPlotName    = "Waveform";
                constexpr const char* s_overlayPlotName     = "Overlay";
                constexpr const char* s_volumeAxisName      = "Channel";
                constexpr const char* s_timeAxisName        = "Time";
                constexpr const char* s_decibelAxisName     = "dB";
                constexpr const char* s_lufsAxisName        = "LU";

                constexpr float s_sampleResolutionThreshold = 0.1f;
                constexpr float s_highResolutionThreshold   = 1.0f;
                constexpr float s_mediumResolutionThreshold = 10.0f;
                constexpr float s_lowResolutionThreshold    = 100.0f;

                constexpr float s_linearVolumeMax           = 1.0f;
                constexpr float s_linearVolumeMinMaxDelta   = 2.0f;

                constexpr float s_decibelVolumeMax          = 0.0f;

                constexpr float s_lufsVolumeMax             = 0.0f;
                constexpr float s_lufsVolumeMin             = -48.0f;
                constexpr float s_lufsMidPoint              = (s_lufsVolumeMin + s_lufsVolumeMax) / 2.0f;

                static std::vector<double> dbAxisTickValues         = {-96.0, -75.0, -60.0, -45.0, -30.0, -24.0, -18.0, -12.0, -6.0, 0.0};
                static std::vector<const char*> dbAxisTickLabels    = {"-96", "-75", "-60", "-45", "-30", "-24", "-18", "-12", "-6", "0"};

                static std::vector<double> lufsAxisTickValues       = {-48.0, -36.0, -31.0, -27.0, -24.0, -23.0, -18.0, -16.0, -14.0, -12.0, -6.0, 0.0};
                static std::vector<const char*> lufsAxisTickLabels  = {"-48", "-36", "-31", "-27", "-24", "-23", "-18", "-16", "-14", "-12", "-6", "0"};

                static std::vector<double> linearAxisOneChannelTickValues           = {0.0};
                static std::vector<const char*> linearAxisOneChannelTickLabels      = {"C"};

                static std::vector<double> linearAxisTickValues                     = {0.0, -2.0, -4.0};
                static std::vector<const char*> linearAxisTickLabels                = {"L", "R", "C"};

                constexpr float s_timeMinValue              = 0.0f;

                float plotLimitLeft                         = ImGui::GetStateStorage()->GetFloat(ImGui::GetID(s_plotLimitMinName));
                float plotLimitRight                        = ImGui::GetStateStorage()->GetFloat(ImGui::GetID(s_plotLimitMaxName));
                float minimumDecibelValue                   = ImGui::GetStateStorage()->GetFloat(ImGui::GetID(s_minimumDecibelName), -96.0f);
                bool renderMidSide                          = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderMidSideName));
                bool renderLows                             = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderLowsName), true);
                bool renderMids                             = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderMidsName), true);
                bool renderHighs                            = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderHighsName), true);
                int overlayMode                             = ImGui::GetStateStorage()->GetInt(ImGui::GetID(s_overlayModeName), 0);
                bool renderMomentary                         = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderMomentaryName), true);
                bool renderShortterm                         = ImGui::GetStateStorage()->GetBool(ImGui::GetID(s_renderShorttermName), true);

                const auto& waveformLods    = audioSubsystem->get_ui_waveform_lods(m_filePath, m_fileDuration);

                if (!waveformLods.has_data())
                {
                    return;
                }

                const float plotTimeWidth = plotLimitRight - plotLimitLeft;

                bool sampleRes = false;

                const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* waveformCache = &waveformLods.m_cache.thumbnailRes.get_cached_data();
                const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* peakCache     = &waveformLods.m_cache.thumbnailRes.get_cached_data();

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

                if (!waveformCache || !peakCache)
                {
                    return;
                }

                const bool dropLufsMax     = false;
                const bool raiseLufsMin    = false;
                const float lufsAxisMin         = raiseLufsMin ? s_lufsMidPoint : s_lufsVolumeMin;
                const float lufsAxisMax         = dropLufsMax ? s_lufsMidPoint : s_lufsVolumeMax;

                const std::size_t channels = waveformCache && waveformCache->has_data() ? waveformCache->m_cache.lodWaveform.channelFrames.size() : 0;

                ImGui::SetCursorScreenPos(m_audioBackground.get_element_rect().GetTL());

                if (ImPlot::BeginPlot(fmt::format("{}##{}", s_waveformPlotName, m_filePath.filename().string()).c_str(), ImVec2(-1, m_audioBackground.get_element_rect().GetHeight()), ImPlotFlags_NoTitle  | ImPlotFlags_NoMouseText | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMenus))
                {
                    ImPlot::SetupLegend(ImPlotLocation_South, ImPlotLegendFlags_Outside | ImPlotLegendFlags_Horizontal);

                    ImPlot::SetupMouseText(ImPlotLocation_SouthEast, ImPlotMouseTextFlags_NoAuxAxes);

                    ImPlot::SetupAxis(ImAxis_X1, s_timeAxisName, ImPlotAxisFlags_None);
                    ImPlot::SetupAxisFormat(ImAxis_X1, time_formatter);
                    ImPlot::SetupAxisLimits(ImAxis_X1, s_timeMinValue, m_fileDuration);
                    ImPlot::SetupAxisLimitsConstraints(ImAxis_X1, s_timeMinValue, m_fileDuration);

                    const float minYValue   = s_linearVolumeMax - (s_linearVolumeMinMaxDelta * channels);
                    const float mindBYValue = minimumDecibelValue;

                    ImPlot::SetupAxis(ImAxis_Y1, s_volumeAxisName, ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, minYValue, s_linearVolumeMax, ImPlotCond_Always);
                    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y1, minYValue, s_linearVolumeMax);

                    switch (channels)
                    {
                        case 1:
                        {
                            ImPlot::SetupAxisTicks(ImAxis_Y1, linearAxisOneChannelTickValues.data(), linearAxisOneChannelTickValues.size(), linearAxisOneChannelTickLabels.data());
                            break;
                        }
                        case 2:
                        case 3:
                        default:
                        {

                            ImPlot::SetupAxisTicks(ImAxis_Y1, linearAxisTickValues.data(), linearAxisTickValues.size(), linearAxisTickLabels.data());
                            break;
                        }
                    }

                    ImPlot::SetupAxis(ImAxis_Y2, s_decibelAxisName, ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxisLimits(ImAxis_Y2, mindBYValue, 0.0, ImPlotCond_Always);
                    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y2, mindBYValue, s_decibelVolumeMax);
                    ImPlot::SetupAxisTicks(ImAxis_Y2, dbAxisTickValues.data(), dbAxisTickValues.size(), dbAxisTickLabels.data());

                    ImPlot::SetupAxis(ImAxis_Y3, s_lufsAxisName, ImPlotAxisFlags_AuxDefault | ImPlotAxisFlags_Lock);
                    ImPlot::SetupAxisLimits(ImAxis_Y3, lufsAxisMin, lufsAxisMax);
                    ImPlot::SetupAxisLimitsConstraints(ImAxis_Y3, lufsAxisMin, lufsAxisMax);
                    ImPlot::SetupAxisTicks(ImAxis_Y3, lufsAxisTickValues.data(), lufsAxisTickValues.size(), lufsAxisTickLabels.data());

                    using fill_channel_points_callback_type = std::function<void(int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)>;

                    const auto get_render_points_at_cache_resolution = [fileDuration = m_fileDuration, plotLimitLeft, plotLimitRight](const gluten::audio_subsystem::waveform_lod_cache_type::cached_data* cache) -> int
                        {
                            if (cache && cache->has_data())
                            {
                                const int numWaveformPoints                      = (int)(fileDuration * cache->m_cache.resolution);
                                const int plotLimitLeftAsPointIndexAtCurrentRes  = (plotLimitLeft / fileDuration) * numWaveformPoints;
                                const int plotLimitRightAsPointIndexAtCurrentRes = (std::clamp((plotLimitRight + 1.0f), 0.0f, (float)fileDuration) / fileDuration) * numWaveformPoints;

                                return plotLimitRightAsPointIndexAtCurrentRes - plotLimitLeftAsPointIndexAtCurrentRes;
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
                            const int maxIndex                               = fileDuration * numWaveformPoints;

                            const int numberOfPointsAtCurrentResolution = plotLimitRightAsPointIndexAtCurrentRes - plotLimitLeftAsPointIndexAtCurrentRes;

                            int point = plotLimitLeftAsPointIndexAtCurrentRes;
                            int index = 0;

                            for (; point < numWaveformPoints && point <= plotLimitRightAsPointIndexAtCurrentRes && index < numberOfPointsAtCurrentResolution; ++point, ++index)
                            {
                                const int clampedIndex = std::clamp(point, 0, numWaveformPoints - 1);

                                const double xPosition  = (double)point / (numWaveformPoints) * fileDuration;

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

                    for (int channel = 0; channel < channels; ++channel)
                    {
                        const int numberOfPointsAtCurrentResolution = get_render_points_at_cache_resolution(waveformCache);

                        std::vector<ImVec2> upperPoints(sampleRes ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                        std::vector<ImVec2> lowerPoints(sampleRes ? 0 : numberOfPointsAtCurrentResolution, ImVec2());
                        std::vector<ImVec2> samplePoints(sampleRes ? numberOfPointsAtCurrentResolution : 0, ImVec2());

                        fill_channel_points(waveformCache, channel, [&upperPoints, &lowerPoints, &samplePoints, &renderMidSide, &channel, &channels, &sampleRes](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                            {
                                if (renderMidSide && channels == 2 && !sampleRes)
                                {
                                    upperPoints[index] = (ImVec2(xPosition, (channel == 0 ? stereoData.midMax : stereoData.sideMax) - (2.0f * channel)));
                                    lowerPoints[index] = (ImVec2(xPosition, (channel == 0 ? stereoData.midMin : stereoData.sideMin) - (2.0f * channel)));
                                }
                                else
                                {
                                    if (sampleRes)
                                    {
                                        samplePoints[index] = (ImVec2(xPosition, channelFrame.sample - (2.0f * channel)));
                                    }
                                    else
                                    {
                                        upperPoints[index] = (ImVec2(xPosition, channelFrame.max - (2.0f * channel)));
                                        lowerPoints[index] = (ImVec2(xPosition, channelFrame.min - (2.0f * channel)));
                                    }
                                }
                            });

                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);

                        static ImPlotSpec waveformSpec;
                        waveformSpec.LineColor = gluten::theme::hex_to_imgui_imvec4(0x4c72b0);
                        waveformSpec.FillColor = waveformSpec.LineColor;

                        if (sampleRes)
                        {
                            ImPlot::PlotStairsG("Waveform", vec2_to_plot_point, samplePoints.data(), samplePoints.size(), waveformSpec);
                        }
                        else
                        {
                            ImPlot::PlotShadedG("Waveform", vec2_to_plot_point, upperPoints.data(), vec2_to_plot_point, lowerPoints.data(), lowerPoints.size(), waveformSpec);
                            ImPlot::PlotLineG("Waveform", vec2_to_plot_point, upperPoints.data(), upperPoints.size(), waveformSpec);
                            ImPlot::PlotLineG("Waveform", vec2_to_plot_point, lowerPoints.data(), lowerPoints.size(), waveformSpec);
                        }
                    }

                    const int numberOfPeakPoints = get_render_points_at_cache_resolution(peakCache);

                    std::vector<ImVec2> peakDecibelPoints(numberOfPeakPoints, ImVec2());
                    std::vector<ImVec2> lowDecibelPoints(numberOfPeakPoints, ImVec2());
                    std::vector<ImVec2> midDecibelPoints(numberOfPeakPoints, ImVec2());
                    std::vector<ImVec2> highDecibelPoints(numberOfPeakPoints, ImVec2());
                    std::vector<ImVec2> rmsPoints(numberOfPeakPoints, ImVec2());

                    fill_channel_points(peakCache, 0, [&minimumDecibelValue, &peakDecibelPoints, &lowDecibelPoints, &midDecibelPoints, &highDecibelPoints, &rmsPoints](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                        {
                            if (globalFrameCache.get_cached_data().has_data())
                            {
                                const auto& globalFrameData = globalFrameCache.get_cached_data().m_cache[point];

                                const float rmsDecibel        = std::max(ma_volume_linear_to_db(globalFrameData.rms), minimumDecibelValue + 1.0f);
                                const float peakDecibel       = std::max(ma_volume_linear_to_db(globalFrameData.channelSumAverage), minimumDecibelValue + 1.0f);
                                const float lowDecibel        = std::max(ma_volume_linear_to_db(globalFrameData.lowAverage), minimumDecibelValue + 1.0f);
                                const float midDecibel        = std::max(ma_volume_linear_to_db(globalFrameData.midAverage), minimumDecibelValue + 1.0f);
                                const float highDecibel       = std::max(ma_volume_linear_to_db(globalFrameData.highAverage), minimumDecibelValue + 1.0f);

                                peakDecibelPoints[index] = (ImVec2(xPosition, peakDecibel));
                                lowDecibelPoints[index]  = (ImVec2(xPosition, lowDecibel));
                                midDecibelPoints[index]  = (ImVec2(xPosition, midDecibel));
                                highDecibelPoints[index] = (ImVec2(xPosition, highDecibel));
                                rmsPoints[index]         = (ImVec2(xPosition, rmsDecibel));
                            }
                        });

                    const int numberOfLufsPoints = get_render_points_at_cache_resolution(&waveformLods.m_cache.thumbnailRes.get_cached_data());

                    std::vector<ImVec2> momentaryPoints(numberOfLufsPoints, ImVec2());
                    std::vector<ImVec2> shorttermPoints(numberOfLufsPoints, ImVec2());
                    std::vector<ImVec2> shorttermClampedPoints(numberOfLufsPoints, ImVec2());
                    std::vector<ImU32> shorttermLineColors(numberOfLufsPoints, ImGui::ColorConvertFloat4ToU32(gluten::theme::supportInfo));

                    fill_channel_points(&waveformLods.m_cache.thumbnailRes.get_cached_data(), 0, [&minimumDecibelValue, &momentaryPoints, &shorttermPoints, &shorttermClampedPoints](int index, double xPosition, int point, const gluten::channel_frame& channelFrame, const gluten::stereo_data& stereoData, const gluten::waveform::global_frame_cache_type& globalFrameCache)
                        {
                            if (globalFrameCache.get_cached_data().has_data())
                            {
                                const auto& globalFrameData = globalFrameCache.get_cached_data().m_cache[point];

                                const float& shortterm        = globalFrameData.lufs.shortterm;
                                const float& momentary        = globalFrameData.lufs.momentary;

                                momentaryPoints[index]          = (ImVec2(xPosition, momentary));
                                shorttermPoints[index]          = (ImVec2(xPosition, shortterm));
                                shorttermClampedPoints[index]   = (ImVec2(xPosition, std::min(shortterm, -24.0f)));
                            }
                        });

                    for (int i = 0; i < shorttermPoints.size(); ++i)
                    {
                        if (shorttermPoints[i].y > -24.0f)
                        {
                            shorttermLineColors[i] = ImGui::ColorConvertFloat4ToU32(gluten::theme::hex_to_imgui_imvec4(0xc46c5e));
                        }
                        else
                        {
                            shorttermLineColors[i] = ImGui::ColorConvertFloat4ToU32(gluten::theme::hex_to_imgui_imvec4(0xa3b5c8));
                        }
                    }

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y2);

                    static ImPlotSpec peakSpec;
                    static ImPlotSpec rmsSpec;
                    static ImPlotSpec lowSpec;
                    static ImPlotSpec midSpec;
                    static ImPlotSpec highSpec;
                    static ImPlotSpec momentarySpec;
                    static ImPlotSpec shorttermSpec;
                    static ImPlotSpec shorttermOverSpec;
                    static ImPlotSpec shorttermLineSpec;

                    using namespace gluten::theme;

                    peakSpec.LineColor = adjust_alpha(supportWarning, 0.33f);
                    rmsSpec.LineColor  = adjust_alpha(supportInfo, 0.5f);
                    lowSpec.LineColor  = adjust_alpha(supportError, 0.5f);
                    midSpec.LineColor  = adjust_alpha(supportWarning, 0.5f);
                    highSpec.LineColor = adjust_alpha(supportSuccess, 0.5f);
                    momentarySpec.LineColor         = hex_to_imgui_imvec4(0xeaecef);
                    shorttermSpec.FillColor         = adjust_alpha(hex_to_imgui_imvec4(0x3f4d63), 0.33f);
                    shorttermOverSpec.FillColor     = adjust_alpha(hex_to_imgui_imvec4(0x5a3431), 0.33f);
                    shorttermLineSpec.LineColors    = shorttermLineColors.data();

                    lowSpec.LineWeight = midSpec.LineWeight = highSpec.LineWeight = 2.0f;
                    momentarySpec.LineWeight = shorttermLineSpec.LineWeight = 2.0f;

                    if (ImPlot::BeginItem(s_overlayPlotName, highSpec, highSpec.LineColor))
                    {
                        ImPlot::EndItem();
                    }

                    if (overlayMode == 0)
                    {
                        if (renderLows)
                        {
                            ImPlot::PlotLineG(s_overlayPlotName, vec2_to_plot_point, lowDecibelPoints.data(), lowDecibelPoints.size(), lowSpec);
                        }

                        if (renderMids)
                        {
                            ImPlot::PlotLineG(s_overlayPlotName, vec2_to_plot_point, midDecibelPoints.data(), midDecibelPoints.size(), midSpec);
                        }

                        if (renderHighs)
                        {
                            ImPlot::PlotLineG(s_overlayPlotName, vec2_to_plot_point, highDecibelPoints.data(), highDecibelPoints.size(), highSpec);
                        }
                    }
                    else if (overlayMode == 1)
                    {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);

                        if (renderShortterm)
                        {
                            ImPlot::PlotShadedG(s_overlayPlotName, vec2_to_plot_point, shorttermPoints.data(), vec2_to_min_db_plot_point, shorttermPoints.data(), shorttermPoints.size(), shorttermOverSpec);
                            ImPlot::PlotLineG(s_overlayPlotName, vec2_to_plot_point, shorttermPoints.data(), shorttermPoints.size(), shorttermLineSpec);
                        }

                        if (renderMomentary)
                        {
                            ImPlot::PlotLineG(s_overlayPlotName, vec2_to_plot_point, momentaryPoints.data(), momentaryPoints.size(), momentarySpec);
                        }

                        if (ImPlot::BeginItem(s_overlayPlotName, highSpec, highSpec.LineColor))
                        {
                            ImPlot::EndItem();
                        }
                    }

                    if (ImPlot::BeginLegendPopup(s_waveformPlotName))
                    {
                        if (channels == 2)
                        {
                            ImGui::Checkbox("Render Mid/Side", &renderMidSide);
                        }

                        ImPlot::EndLegendPopup();
                    }

                    if (ImPlot::BeginLegendPopup(s_overlayPlotName))
                    {
                        static const char* labels[2] = {"Multi-Band", "LUFS"};

                        if (ImGui::BeginCombo("Overlay Mode", labels[overlayMode]))
                        {
                            if (ImGui::Selectable(labels[0], overlayMode == 0))
                            {
                                overlayMode = 0;
                            }

                            if (ImGui::Selectable(labels[1], overlayMode == 1))
                            {
                                overlayMode = 1;
                            }

                            ImGui::EndCombo();
                        }

                        if (overlayMode == 0)
                        {
                            ImGui::Checkbox("Render Lows", &renderLows);
                            ImGui::Checkbox("Render Mids", &renderMids);
                            ImGui::Checkbox("Render Highs", &renderHighs);
                        }
                        else if (overlayMode == 1)
                        {
                            ImGui::Checkbox("Render Momentary", &renderMomentary);
                            ImGui::Checkbox("Render Shortterm", &renderShortterm);
                        }


                        ImPlot::EndLegendPopup();
                    }

                    if (waveformLods.m_cache.sampleRes.get_cached_data().has_data())
                    {
                        ImPlot::SetAxes(ImAxis_X1, ImAxis_Y3);

                        ImPlot::TagY(waveformLods.m_cache.sampleRes.get_cached_data().m_cache.lodWaveform.lufs.integrated, gluten::theme::supportInfo, fmt::format("LUFS-I: {:.1f}", waveformLods.m_cache.sampleRes.get_cached_data().m_cache.lodWaveform.lufs.integrated).c_str());
                    }

                    ImPlot::SetAxes(ImAxis_X1, ImAxis_Y1);

                    const double scaledFilePosition = m_filePosition;
                    ImPlot::PlotInfLines("##Playhead", &scaledFilePosition, 1, {ImPlotProp_LineWeight, 3.0f, ImPlotProp_LineColor, is_playing() ? gluten::theme::supportSuccess : gluten::theme::supportError});

                    render_waveform_overlay(plotTimeWidth);

                    if (ImPlot::IsPlotHovered())
                    {
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

                ImGui::GetStateStorage()->SetFloat(ImGui::GetID(s_plotLimitMinName), plotLimitLeft);
                ImGui::GetStateStorage()->SetFloat(ImGui::GetID(s_plotLimitMaxName), plotLimitRight);
                ImGui::GetStateStorage()->SetFloat(ImGui::GetID(s_minimumDecibelName), minimumDecibelValue);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderMidSideName), renderMidSide);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderLowsName), renderLows);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderMidsName), renderMids);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderHighsName), renderHighs);
                ImGui::GetStateStorage()->SetInt(ImGui::GetID(s_overlayModeName), overlayMode);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderMomentaryName), renderMomentary);
                ImGui::GetStateStorage()->SetBool(ImGui::GetID(s_renderShorttermName), renderShortterm);
            }
        }
    }
}  // namespace gluten
