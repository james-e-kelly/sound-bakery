#include "audio_meter_widget.h"

#include "gluten/elements/layouts/layout.h"
#include "gluten/elements/slider.h"
#include "imgui.h"
#include "implot.h"

#include <algorithm>

static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.y, lhs.y - rhs.y); }

static ImVec2 plotOffset(100, 70);

namespace audio_meter_utils
{
    // without an offset, the bar graph renders down, instead of up.
    // We offset the volume to show positive values but label it as negative
    constexpr double volume_offset     = 100.0;
    constexpr std::size_t meter_ticks  = 14;
    constexpr double bar_width         = 0.9;
    constexpr float attack_lerp_speed  = 0.1;
    constexpr float release_lerp_speed = 0.9;

    // @TODO Find some constexpr version of this
    // The std::array is stopping us from using constexpr fully
    struct audio_meter_draw_info
    {
        audio_meter_draw_info()
        {
            std::transform(raw_volume_values.begin(), raw_volume_values.end(), volume_labels_storage.begin(),
                           [](double value) { return std::to_string(int(value)); });

            std::transform(volume_labels_storage.begin(), volume_labels_storage.end(), volume_labels.begin(),
                           [](std::string& value) { return value.c_str(); });

            std::transform(raw_volume_values.begin(), raw_volume_values.end(), offset_volume_values.begin(),
                           [](double value) { return value + volume_offset; });
        }

        // Values we want to show the user
        std::array<double, meter_ticks> raw_volume_values{-100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0,
                                                          -30.0,  -20.0, -10.0, -5.0,  0.0,   5.0,   10.0};

        // Strings to display to the user
        std::array<const char*, meter_ticks> volume_labels;

        std::array<double, meter_ticks> offset_volume_values;

    private:
        std::array<std::string, meter_ticks> volume_labels_storage;
    };

    static audio_meter_draw_info draw_info;
    static const double min_volume        = draw_info.raw_volume_values.front();
    static const double max_volume        = draw_info.raw_volume_values.back();
    static const double offset_min_volume = draw_info.offset_volume_values.front();
    static const double offset_max_volume = draw_info.offset_volume_values.back();
    static double rendered_min_volume     = offset_min_volume;
    static double rendered_max_volume     = offset_max_volume;

    static double min_volume_slider_min() { return offset_min_volume; }
    static double min_volume_slider_max() { return rendered_max_volume - 5.0; }

    static double max_volume_slider_min() { return rendered_min_volume + 10.0; }
    static double max_volume_slider_max() { return offset_max_volume; }

    constexpr const char* channel_labels[] = {"L", "R", "C", "RL", "RR", "ML", "MR"};
}  // namespace audio_meter_utils

void audio_meter_widget::start()
{
    m_rmsVolumes.fill(audio_meter_utils::rendered_min_volume);

    if (sc_node_group* const masterNodeGroup = sbk::engine::system::get()->masterNodeGroup)
    {
        sc_node_group_get_dsp(masterNodeGroup, SC_DSP_TYPE_METER, &m_meterDsp);
    }
}

void audio_meter_widget::render()
{
    if (ImGui::Begin("Meter"))
    {
        const ImVec2 windowSize = ImGui::GetWindowSize();

        gluten::layout settingsBarLayout(gluten::element::anchor_preset::stretch_top);
        settingsBarLayout.get_element_anchor().maxOffset.y = 64;
        settingsBarLayout.set_element_frame_padding();
        settingsBarLayout.render_window();

        gluten::slider<double> minVolumeSlider("##Min Volume", &audio_meter_utils::rendered_min_volume,
                                               audio_meter_utils::min_volume_slider_min(),
                                               audio_meter_utils::min_volume_slider_max());
        settingsBarLayout.render_layout_element_percent_horizontal(&minVolumeSlider, 0.49f);
        settingsBarLayout.render_layout_element_percent_horizontal(nullptr, 0.02f);

        gluten::slider<double> maxVolumeSlider("##Max Volume", &audio_meter_utils::rendered_max_volume,
                                               audio_meter_utils::max_volume_slider_min(),
                                               audio_meter_utils::max_volume_slider_max());
        settingsBarLayout.render_layout_element_percent_horizontal(&maxVolumeSlider, 0.49f);

        settingsBarLayout.finish_layout();

        if (ImPlot::BeginPlot(
                "Meter", ImVec2(-1, windowSize.y - plotOffset.y - settingsBarLayout.get_element_rect().GetSize().y),
                ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs | ImPlotFlags_NoFrame |
                    ImPlotFlags_NoLegend))
        {

            ma_uint32 channels = 0;

            if (m_meterDsp)
            {
                channels = ma_node_get_output_channels(m_meterDsp->state->userData, 0);

                for (std::size_t index = 0; index < channels; ++index)
                {
                    float channelVolume = 0;
                    if (sc_dsp_get_metering_info(m_meterDsp, index, SC_DSP_METER_RMS, &channelVolume) == MA_SUCCESS)
                    {
                        const float convertedVolume =
                            std::clamp(ma_volume_linear_to_db(channelVolume) + audio_meter_utils::volume_offset,
                                       audio_meter_utils::rendered_min_volume, audio_meter_utils::rendered_max_volume);
                        const float currentVolume = m_rmsVolumes[index];

                        const float lerpedVolume =
                            std::lerp(currentVolume, convertedVolume,
                                      convertedVolume < currentVolume ? audio_meter_utils::attack_lerp_speed
                                                                      : audio_meter_utils::release_lerp_speed);
                        m_rmsVolumes[index] = lerpedVolume;
                    }
                }
            }

            ImPlot::SetupAxes("", "RMS Volume", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, audio_meter_utils::rendered_min_volume,
                                    audio_meter_utils::rendered_max_volume, ImPlotCond_Always);
            ImPlot::SetupAxisTicks(ImAxis_X1, 0.0, channels - 1.0, channels, audio_meter_utils::channel_labels);
            ImPlot::SetupAxisTicks(ImAxis_Y1, audio_meter_utils::draw_info.offset_volume_values.data(),
                                   audio_meter_utils::meter_ticks, audio_meter_utils::draw_info.volume_labels.data());
            ImPlot::PlotBars("My Bar Plot", m_rmsVolumes.data(), channels, audio_meter_utils::bar_width);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}
