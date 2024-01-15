#include "MethodDrawer.h"

#include "imgui.h"
#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/editor/editor_defines.h"

using MethodIndex    = uint32_t;
using ParameterIndex = uint32_t;

static SB_ID s_currentInstance;
static std::unordered_map<MethodIndex,
                          std::unordered_map<ParameterIndex, rttr::variant>>
    s_parameterCache;

void MethodDrawer::DrawObject(rttr::type type, rttr::instance instance)
{
    if (!instance.get_type().is_derived_from(
            rttr::type::get<SB::Core::Object>()))
    {
        assert(false);
        return;
    }

    SB::Core::DatabaseObject* object =
        instance.try_convert<SB::Core::DatabaseObject>();

    if (s_currentInstance != object->getDatabaseID())
    {
        s_currentInstance = object->getDatabaseID();
        s_parameterCache.clear();
    }

    MethodIndex methodIndex = 0;

    for (const rttr::method& method : type.get_methods())
    {
        std::vector<rttr::argument> arguments;

        for (const rttr::parameter_info& parameter :
             method.get_parameter_infos())
        {
            const rttr::type paramType = parameter.get_type();

            rttr::variant cachedParam =
                s_parameterCache[s_currentInstance][methodIndex];

            if (paramType.is_enumeration())
            {
                const rttr::enumeration paramEnum = paramType.get_enumeration();

                if (!cachedParam.is_valid())
                {
                    cachedParam = *(paramEnum.get_values().begin());
                }

                const rttr::string_view previewName =
                    paramEnum.value_to_name(cachedParam);
                rttr::string_view labelName = parameter.get_name();
                if (labelName.empty())
                {
                    labelName = "Unknown";
                }

                if (ImGui::BeginCombo(labelName.data(), previewName.data()))
                {
                    for (const rttr::string_view& enumValueName :
                         paramEnum.get_names())
                    {
                        rttr::variant enumValue =
                            paramEnum.name_to_value(enumValueName);
                        bool selected = enumValue == cachedParam;
                        if (ImGui::Selectable(enumValueName.data(), &selected))
                        {
                            cachedParam = enumValue;
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            arguments.push_back(cachedParam);
            s_parameterCache[s_currentInstance][methodIndex] = cachedParam;
        }

        if (ImGui::Button(method.get_name().data()))
        {
            switch (arguments.size())
            {
                case 0:
                    method.invoke(instance);
                case 1:
                    method.invoke(instance, arguments[0]);
                    break;
                case 2:
                    method.invoke(instance, arguments[0], arguments[1]);
                    break;
                case 3:
                    method.invoke(instance, arguments[0], arguments[1],
                                  arguments[2]);
                    break;
                case 4:
                    method.invoke(instance, arguments[0], arguments[1],
                                  arguments[2], arguments[3]);
                    break;
                case 5:
                    method.invoke(instance, arguments[0], arguments[1],
                                  arguments[2], arguments[3], arguments[4]);
                    break;
                case 6:
                    method.invoke(instance, arguments[0], arguments[1],
                                  arguments[2], arguments[3], arguments[4],
                                  arguments[5]);
                    break;
            }
        }

        ++methodIndex;
    }
}
