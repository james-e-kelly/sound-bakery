#include "add_effect_button.h"

#include "app/app.h"
#include "managers/project_manager.h"
#include "sound_bakery/node/node.h"

namespace add_effect_button_utils
{
    static constexpr const char* effectPopupName = "Effect_Popup";
}

auto add_effect_button::render_element(const ImRect& elementBox) -> bool
{ 
	if (std::shared_ptr<project_manager> projectManager = gluten::app::get()->get_manager_by_class<project_manager>())
    {
        selection& selection = projectManager->get_selection();
        if (sbk::core::object* selected = selection.get_selected())
        {
            if (selected->getType().is_derived_from<sbk::engine::node_base>())
            {
                if (ImGui::Button("Add Effect..."))
                {
                    ImGui::OpenPopup(add_effect_button_utils::effectPopupName);
                }

                if (ImGui::BeginPopup(add_effect_button_utils::effectPopupName))
                {
                    if (ImGui::BeginMenu("Default"))
                    {
                        const rttr::type basicDspType               = rttr::type::get<sc_dsp_type>();
                        const rttr::enumeration basicDspEnumeration = basicDspType.get_enumeration();

                        for (const rttr::string_view& enumValueName : basicDspEnumeration.get_names())
                        {
                            rttr::variant enumValue = basicDspEnumeration.name_to_value(enumValueName);
                            if (ImGui::MenuItem(enumValueName.data()))
                            {
                                selected->try_convert_object<sbk::engine::node>()->addEffect(
                                    enumValue.get_value<sc_dsp_type>());
                            }
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("CLAP"))
                    {
                        ma_uint32 clapCount = 0;
                        if (sc_system_clap_get_count(sbk::engine::system::get(), &clapCount) == MA_SUCCESS)
                        {
                            for (int index = 0; index < clapCount; ++index)
                            {
                                sc_clap* clapPlugin = nullptr;
                                if (sc_system_clap_get_at(sbk::engine::system::get(), index, &clapPlugin) ==
                                    MA_SUCCESS)
                                {
                                    if (const clap_plugin_descriptor_t* const pluginDescriptor =
                                            clapPlugin->pluginFactory->get_plugin_descriptor(
                                                clapPlugin->pluginFactory, 0))
                                    {
                                        if (ImGui::MenuItem(pluginDescriptor->name))
                                        {
                                        }
                                    }
                                }
                            }
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::EndPopup();
                }
            }
        }
    }

	return false; 
}