#pragma once

#include <utility>

namespace SB::Editor
{
    using MinMax = std::pair<float, float>;

    enum class METADATA_KEY
    {
        Payload,
        MinMax,
        Readonly
    };

    inline std::string PayloadObject = "OBJECT";
    inline std::string PayloadSound = "SOUND";
    inline std::string PayloadBus = "BUS";
    inline std::string PayloadIntParam = "INT_PARAM";
    inline std::string PayloadIntParamValue = "INT_PARAM_VALUE";
    inline std::string PayloadFloatParam = "FLOAT_PARAM";
}