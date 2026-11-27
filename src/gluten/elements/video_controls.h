#pragma once

#include "gluten/elements/audio_controls.h"

namespace gluten
{
    class video_element;

    /**
     * @brief The transport every Gluten video app shares, styled to match audio_transport: stop, frame step,
     * play/pause, loop, frame timecode, timecode/frames toggle, frame overlay toggles, speed and volume.
     * Speed, volume, mute and looping are the audio subsystem's preview settings, so they carry across audio
     * and video files alike.
     *
     * Wraps onto a second row when the width can't fit everything on one.
     */
    class video_transport
    {
    public:
        static auto get_height(float width, float extrasWidth = 0.0f) -> float;
        static auto render(const ImRect& area, video_element& video, const transport_extras& extras = {}) -> void;
    };
}  // namespace gluten
