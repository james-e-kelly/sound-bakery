#include "audio_meter_widget.h"

#include "imgui.h"
#include "implot.h"
#include "sound_bakery/system.h"

#include <algorithm>

static ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.y, lhs.y - rhs.y); }

static ImVec2 plotOffset(100, 60);

namespace audio_meter_utils
{
    // without an offset, the bar graph renders down, instead of up. 
    // We offset the volume to show positive values but label it as negative
    constexpr double volume_offset = 100.0;  
    constexpr std::size_t meter_ticks = 14;

    // @TODO Find some constexpr version of this
    // The std::array is stopping us from using constexpr fully
    struct audio_meter_draw_info
    {
        constexpr audio_meter_draw_info() 
        { 
            std::transform(
                raw_volume_values.begin(), raw_volume_values.end(), 
                volume_labels_storage.begin(), 
                [](double value) { return std::to_string(int(value)); });

            std::transform(
                volume_labels_storage.begin(), volume_labels_storage.end(), 
                volume_labels.begin(),
                [](std::string& value) { return value.c_str(); });

            std::transform(
                raw_volume_values.begin(), raw_volume_values.end(), 
                offset_volume_values.begin(),
                [](double value) { return value + volume_offset; });
        }

        // Values we want to show the user
        std::array<double, meter_ticks> raw_volume_values{
            -100.0, -90.0, -80.0, -70.0, -60.0, -50.0, -40.0, 
            -30.0, -20.0, -10.0, -5.0, 0.0, 5.0, 10.0};

        // Strings to display to the user
        std::array<const char*, meter_ticks> volume_labels;

        std::array<double, meter_ticks> offset_volume_values;

    private:
        std::array<std::string, meter_ticks> volume_labels_storage;
    };

    static audio_meter_draw_info draw_info;  
    static double min_volume = draw_info.raw_volume_values.front();
    static double max_volume = draw_info.raw_volume_values.back();
    static double offset_min_volume = draw_info.offset_volume_values.front();
    static double offset_max_volume = draw_info.offset_volume_values.back();
}

void audio_meter_widget::start()
{
    if (sc_node_group* const masterNodeGroup = sbk::engine::system::get()->masterNodeGroup)
    {
        sc_node_group_get_dsp(masterNodeGroup, SC_DSP_TYPE_METER, &m_meterDsp);
    }
}

void audio_meter_widget::render()
{
    if (ImGui::Begin("Meter"))
    {
        //ImGui::SliderFloat("Y Offset", &plotOffset.y, 0, 1000.0f);

        const ImVec2 windowSize = ImGui::GetWindowSize();

        if (ImPlot::BeginPlot("Meter", ImVec2(-1,windowSize.y - plotOffset.y),
                              ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText | ImPlotFlags_NoInputs |
                                  ImPlotFlags_NoFrame | ImPlotFlags_NoLegend))
        {
            static float rmsValues[5]{0.0f, 96.0f, 96.0f - 3.5f, 54.2f, 60.1f};
            static const char* channelLabels[] = {"L", "R", "C", "RL", "RR", "ML", "MR"};
            static const char* volumeLabels[]  = {"-96", "0", "6"};
            static const double positions[] = {0, 1, 2};
            static const int channelsToShow    = 5;

            
            ImPlot::SetupAxes("", "RMS Volume", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, audio_meter_utils::offset_min_volume,
                                    audio_meter_utils::offset_max_volume, ImPlotCond_Always);
            ImPlot::SetupAxisTicks(ImAxis_X1, 0.0, channelsToShow - 1.0, channelsToShow, channelLabels);
            ImPlot::SetupAxisTicks(ImAxis_Y1, 
                audio_meter_utils::draw_info.offset_volume_values.data(), 
                audio_meter_utils::meter_ticks, audio_meter_utils::draw_info.volume_labels.data());
            ImPlot::PlotBars("My Bar Plot", rmsValues, channelsToShow, 1.0, 0.0,0, -10);
            ImPlot::EndPlot();
        }
    }
    ImGui::End();
}