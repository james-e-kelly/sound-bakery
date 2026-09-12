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
        no_grow,            //< Container cannot have elements added. Helpful when a function needs to add the element and a simple add would create an invalid element
        no_shrink,          //< Container cannot have its elements removed
        draw_when_wrapped,  //< If this type is wrapped inside a database_ptr, render it fully, instead of just being a payload target
        hidden_when_wrapped,//< If we are rendered as a "subobject" (wrapped), don't render the property
        min_version,        //< Minimum version of Sound Bakery needed to load this property. If the file is older than this, the property is skipped
        custom_unit,        //< Property uses custom units like dB
        category,           //< Category/group to render the property in
    };

    enum class render_unit
    {
        none = 0,
        decibel,
    };

    inline std::string PayloadObject        = "OBJECT";
    inline std::string PayloadContainer     = "CONTAINER";
    inline std::string PayloadSound         = "SOUND";
    inline std::string PayloadBus           = "BUS";
    inline std::string PayloadNamedParam    = "NAMED_PARAM";
    inline std::string PayloadIntParamValue = "NAMED_PARAM_VALUE";
    inline std::string PayloadFloatParam    = "FLOAT_PARAM";

    inline std::string_view g_objectCategoryName  = "Object";
    inline std::string_view g_routingCategoryName = "Routing";
    inline std::string_view g_voiceCategoryName   = "Voice";
    inline std::string_view g_effectsCategoryName = "Effects";
    inline std::string_view g_soundCategoryName   = "Sound";
}  // namespace sbk::editor