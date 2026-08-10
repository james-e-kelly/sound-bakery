#pragma once

#include <utility>

namespace sbk::editor
{
    using MinMax = std::pair<float, float>;

    enum class metadata_key
    {
        payload,            //< Drag and drop
        min_max,            //< Has min and max values
        readonly,           //< Cannot be edited
        no_resize,          //< Container cannot be resized. Elements might be writable
        draw_when_wrapped,  //< If this type is wrapped inside a database_ptr, render it fully, instead of just being a payload target
        hidden_when_wrapped //< If we are rendered as a "subobject" (wrapped), don't render the property
    };

    inline std::string PayloadObject        = "OBJECT";
    inline std::string PayloadContainer     = "CONTAINER";
    inline std::string PayloadSound         = "SOUND";
    inline std::string PayloadBus           = "BUS";
    inline std::string PayloadNamedParam    = "NAMED_PARAM";
    inline std::string PayloadIntParamValue = "NAMED_PARAM_VALUE";
    inline std::string PayloadFloatParam    = "FLOAT_PARAM";
}  // namespace sbk::editor