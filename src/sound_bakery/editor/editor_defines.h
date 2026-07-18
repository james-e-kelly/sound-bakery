#pragma once

#include <utility>

namespace sbk::editor
{
    using MinMax = std::pair<float, float>;

    enum class metadata_key
    {
        payload,
        min_max,
        readonly
    };

    inline std::string PayloadObject        = "OBJECT";
    inline std::string PayloadContainer     = "CONTAINER";
    inline std::string PayloadSound         = "SOUND";
    inline std::string PayloadBus           = "BUS";
    inline std::string PayloadNamedParam    = "NAMED_PARAM";
    inline std::string PayloadIntParamValue = "NAMED_PARAM_VALUE";
    inline std::string PayloadFloatParam    = "FLOAT_PARAM";
}  // namespace sbk::editor