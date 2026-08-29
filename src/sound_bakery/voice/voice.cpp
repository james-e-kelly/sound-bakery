#include "voice.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/maths/easing.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/sound/sound.h"

using namespace sbk::engine;

DEFINE_REFLECTION(voice)

auto sbk::engine::voice::play_container(container* container) -> sbk::result<void>
{
    ZoneScoped;
    remove_all();

    gather_children_context context;
    context.sounds.push_back(container->casted_shared_from_this<sbk::engine::container>());
    context.numTimesPlayed = 0;
    context.parameters     = get_owning_game_object()->get_local_parameters();

    runtime* const runtime = get_runtime();

    std::size_t index = 0;
    for (auto childIter = context.sounds.begin(); childIter != context.sounds.end(); ++childIter, ++index)
    {
        m_instances.emplace_back();

        if ((*childIter)->get_object_type() == sound_container::type())
        {
            std::shared_ptr<sound> sharedSound = (*childIter)->casted_shared_from_this<sound_container>()->get_sound();

            if (!sharedSound || sharedSound->get_sound() == nullptr)
            {
                SBK_WARN("{} has no sound to play", (*childIter)->get_object_name());
                continue;
            }

            std::shared_ptr<node_base> outputBus = (*childIter)->get_output_bus();
            sc_node_group* outputNodeGroup       = nullptr;
            float volume                         = (*childIter)->m_volume.get();
            float pitch                          = (*childIter)->m_pitch.get();
            float lowpass                        = (*childIter)->m_lowpass.get();
            float highpass                       = (*childIter)->m_highpass.get();

            std::shared_ptr<node_base> parent = (*childIter)->get_parent();

            while (parent)
            {
                auto sharedParent = parent->casted_shared_from_this<node>();

                // We don't have property overrides so take the bottom-most output bus
                if (!outputBus)
                {
                    outputBus = sharedParent->get_output_bus();
                }

                volume *= sharedParent->m_volume.get();
                pitch *= sharedParent->m_pitch.get();

                // For now, lowwpass and highpass is summed
                // We can add settings later for "take highest"
                lowpass += sharedParent->m_lowpass.get();
                highpass += sharedParent->m_highpass.get();

                parent = parent->get_parent();
            }

            lowpass  = std::clamp(lowpass, (*childIter)->m_lowpass.get_min(), (*childIter)->m_lowpass.get_max());
            highpass = std::clamp(highpass, (*childIter)->m_highpass.get_min(), (*childIter)->m_highpass.get_max());

            if (outputBus)
            {
                std::shared_ptr<bus> sharedBus = std::static_pointer_cast<bus>(outputBus);
                auto getNodeGroupResult        = sharedBus->lock_or_copy_node_group();

                if (getNodeGroupResult.has_value())
                {
                    std::shared_ptr<sc_node_group> nodeGroup = getNodeGroupResult.value();
                    m_outputBusses.push_back(nodeGroup);
                    outputNodeGroup = nodeGroup.get();
                }
            }

            sc_voice_handle handle{};
            if (SBK_REPORT_IF_C(sc_system_play_sound(runtime, sharedSound->get_sound(), &handle, outputNodeGroup, SC_TRUE)))
            {
                m_instances[index].voiceHandle = handle;

                sc_voice_set_volume(runtime, handle, volume);
                sc_voice_set_pitch(runtime, handle, pitch);

                const double cutoffOffset = SC_DSP_CUTOFF_MAX - SC_DSP_CUTOFF_MIN;

                const double lowpassPercentage = sbk::maths::ease_out_cubic(lowpass / 100.0);
                const double lowpassCutoff     = (cutoffOffset - (cutoffOffset * lowpassPercentage)) + SC_DSP_CUTOFF_MIN;

                const double highpassPercentage = sbk::maths::ease_in_cubic(highpass / 100.0);
                const double highpassCutoff     = (cutoffOffset * highpassPercentage) + SC_DSP_CUTOFF_MIN;

                sc_voice_set_lowpass_cutoff(runtime, handle, lowpassCutoff);
                sc_voice_set_highpass_cutoff(runtime, handle, highpassCutoff);

                sc_voice_set_paused(runtime, handle, SC_FALSE);
            }
        }
        else
        {
            const std::size_t containerSizeBeforeGather = context.sounds.size();
            (*childIter)->gather_children_for_play(context);
            const std::size_t containerSizeAfterGather = context.sounds.size();
            const std::size_t childrenSize             = containerSizeAfterGather - containerSizeBeforeGather;

            m_instances.resize(m_instances.size() + childrenSize);

            m_instances[index].childCount = childrenSize;

            // The children spawned from this container go at the end of the array
            // It is not guaranteed for new children to be _next_ in the array
            // For example, container 0 spawns two children
            // We then move forward to container 1, the first child
            // it spawns two more children
            // The new children are at [3,4], not [2,3], which are next in the array
            const std::size_t childIndexStart = context.sounds.size() - 1 - childrenSize;

            for (std::size_t childIndex = 0; childIndex < childrenSize; ++childIndex)
            {
                m_instances[index + childIndexStart + childIndex].parentIndex = index;
            }
        }
    }

    return sbk::make_error(SBK_ERR_BAKERY, "Failed to initialize the voice instance");
}

auto voice::update() -> void
{
    ZoneScoped;

    if (runtime* const runtime = get_runtime())
    {
        for (int index = static_cast<int>(m_instances.size()) - 1; index >= 0; --index)
        {
            if (!m_instances[index].finished)
            {
                unsigned int& childCount     = m_instances[index].childCount;
                sc_voice_handle& voiceHandle = m_instances[index].voiceHandle;
                std::size_t& parentIndex    = m_instances[index].parentIndex;
                sc_bool playing              = SC_FALSE;

                const bool finished = m_instances[index].voiceHandle == 0 ? m_instances[index].childCount == 0 : sc_voice_get_is_playing(runtime, voiceHandle, &playing) != SBK_SUCCESS;

                if (finished)
                {
                    --m_instances[parentIndex].childCount;
                    m_instances[index].finished = true;
                    m_instances[index].voiceHandle = 0;
                }
            }
        }
    }
}

auto sbk::engine::voice::playing_container(container* container) const noexcept -> bool
{
    if (container == nullptr)
    {
        return false;
    }

    for (std::size_t index = 0; index < m_instances.size(); ++index)
    {
        if (m_instances[index].containerReference == container->get_database_id())
        {
            return true;
        }
    }

    return false;
}

auto sbk::engine::voice::is_playing() const -> bool { return m_instances.size() > 0 && !m_instances[0].finished; }

auto sbk::engine::voice::get_owning_game_object() const -> game_object*
{
    return static_cast<sbk::core::object*>(get_owner())->try_convert_object<sbk::engine::game_object>();
}