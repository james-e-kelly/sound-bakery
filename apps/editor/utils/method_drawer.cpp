#include "method_drawer.h"

#include "imgui.h"
#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/util/type_helper.h"

using MethodIndex    = uint32_t;
using ParameterIndex = uint32_t;

static sbk_id s_currentInstance;
static std::unordered_map<MethodIndex,
                          std::unordered_map<ParameterIndex, rttr::variant>>
    s_parameterCache;

void method_drawer::draw_object(rttr::type type, rttr::instance instance)
{
    if (!type.is_derived_from(
            sbk::core::object::type()))
    {
        assert(false);
        return;
    }

    sbk::core::database_object* object = sbk::util::type_helper::getDatabaseObjectFromInstance(instance);

    if (s_currentInstance != object->get_database_id())
    {
        s_currentInstance = object->get_database_id();
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
