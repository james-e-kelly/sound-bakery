#include "PropertyDrawer.h"

#include "imgui.h"
#include "sound_bakery/system.h"
#include "sound_bakery/core/object/object.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/util/type_helper.h"

void PropertyDrawer::DrawObject(rttr::type type, rttr::instance instance)
{
    ImGui::PushID(type.get_name().data());

    if (ImGui::BeginTable(
            "Properties", 2,
                          ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_BordersInnerH |
                              ImGuiTableFlags_SizingStretchProp))
    {
        for (rttr::property property : type.get_properties())
        {
            DrawProperty(property, instance);
        }

        ImGui::EndTable();
    }

    ImGui::PopID();
}

bool PropertyDrawer::DrawProperty(rttr::property property,
                                  rttr::instance instance)
{
    bool edited = false;

    bool readonly = property.is_readonly();

    if (!readonly)
    {
        readonly =
            property.get_metadata(sbk::editor::METADATA_KEY::Readonly).to_bool();
    }

    ImGui::PushID(property.get_name().data());

    rttr::variant propertyValue = property.get_value(instance);

    ImGui::TableNextColumn();

    ImGui::Text("%s: ", property.get_name().data());

    ImGui::TableNextColumn();

    if (readonly)
    {
        DrawReadonlyVariant(propertyValue);
    }
    else if (const rttr::variant payloadString =
                 property.get_metadata(sbk::editor::METADATA_KEY::Payload);
             payloadString.is_valid())
    {
        edited = DrawPayloadDrop(propertyValue, payloadString);
    }
    else
    {
        edited = DrawVariant(
            propertyValue, property.get_name(),
            property.get_metadata(sbk::editor::METADATA_KEY::MinMax));
    }

    if (edited)
    {
        edited = property.set_value(instance, propertyValue);
        assert(edited);
    }

    ImGui::PopID();

    return edited;
}

bool PropertyDrawer::DrawVariant(rttr::variant& variant,
                                 rttr::string_view name,
                                 rttr::variant minMax)
{
    if (!variant.is_valid())
    {
        ImGui::TextUnformatted("Variant is invalid");
        return false;
    }

    const rttr::type type = variant.get_type();
    bool edited           = false;

    assert(type != rttr::type::get<rttr::variant>());

    if (type.is_arithmetic())
    {
        if (type == rttr::type::get<float>())
        {
            float value = variant.to_float();

            std::pair<float, float> floatMinMax =
                minMax.convert<std::pair<float, float>>();

            if (DrawFloat(value, name, floatMinMax))
            {
                variant = value;
                edited  = true;
            }
        }
        else if (type == rttr::type::get<int>())
        {
            int value = variant.to_int();

            std::pair<int, int> intMinMax =
                minMax.convert<std::pair<int, int>>();

            if (DrawInt(value, name, intMinMax))
            {
                variant = value;
                edited  = true;
            }
        }
        else if (type == rttr::type::get<bool>())
        {
            bool value = variant.to_bool();

            if (DrawBool(value, name))
            {
                variant = value;
                edited  = true;
            }
        }
    }
    else if (type.is_sequential_container())
    {
        rttr::variant_sequential_view view = variant.create_sequential_view();

        if (DrawSequentialContainer(view, name))
        {
            edited = true;
        }
    }
    else if (type.is_associative_container())
    {
        rttr::variant_associative_view view = variant.create_associative_view();

        if (DrawAssociateContainer(view, name))
        {
            edited = true;
        }
    }
    else if (type.is_wrapper() && type.get_wrapped_type().is_arithmetic())
    {
        sbk_id id = variant.extract_wrapped_value().convert<sbk_id>();
        rttr::type templateType = *type.get_template_arguments().begin();

        std::string payloadString =
            std::string(sbk::Util::TypeHelper::getPayloadFromType(templateType));

        sbk::core::database_ptr<sbk::core::object> objectPtr(id);

        edited = DrawPayloadDrop(variant, payloadString);
    }
    else if (type.is_pointer())
    {
        DrawReadonlyVariant(variant);
    }
    else if (type.is_wrapper())
    {
        DrawReadonlyVariant(variant);
    }
    else if (type == rttr::type::get<std::string>() ||
             type == rttr::type::get<std::string_view>())
    {
        DrawReadonlyVariant(variant);
    }
    else if (type.is_class())
    {
        if (type == sbk::engine::effect_parameter_description::type())
        {
            sbk::engine::effect_parameter_description effectParamterDescription =
                variant.convert<sbk::engine::effect_parameter_description>();

            switch (effectParamterDescription.m_parameter.type)
            {
                case SC_DSP_PARAMETER_TYPE_FLOAT:
                    sbk::editor::MinMax minMax(
                        effectParamterDescription.m_parameter.floatParameter.min,
                        effectParamterDescription.m_parameter.floatParameter.max);
                    edited = DrawFloat(
                        effectParamterDescription.m_parameter.floatParameter.value,
                        effectParamterDescription.m_parameter.name, minMax);
                    break;
            }

            if (edited)
            {
                variant = effectParamterDescription;
            }
        }
        else if (type == rttr::type::get<sbk::core::float_property>())
        {
            sbk::core::float_property floatProperty =
                variant.convert<sbk::core::float_property>();
            float floatValue = floatProperty.get();

            std::pair<float, float> floatMinMax = floatProperty.get_min_max_pair();

            if (DrawFloat(floatValue, name, floatMinMax))
            {
                floatProperty.set(floatValue);
                variant = floatProperty;
                edited  = true;
            }
        }
        else if (type == rttr::type::get<sbk::core::int_property>())
        {
        }
        else if (type == rttr::type::get<std::filesystem::path>())
        {
            DrawReadonlyVariant(variant);
        }
        else
        {
            if (type.get_properties().size())
            {
                if (DrawMemberObject(variant, name))
                {
                    edited = true;
                }
            }
            else
            {
                ImGui::TextUnformatted("object doesn't have properties");
            }
        }
    }
    else if (type.is_enumeration())
    {
        const rttr::enumeration enumeration = type.get_enumeration();

        const rttr::string_view previewName =
            enumeration.value_to_name(variant);

        if (ImGui::BeginCombo(enumeration.get_name().data(),
                              previewName.data()))
        {
            for (const rttr::string_view& enumValueName :
                 enumeration.get_names())
            {
                rttr::variant enumValue =
                    enumeration.name_to_value(enumValueName);
                bool selected = enumValue == variant;
                if (ImGui::Selectable(enumValueName.data(), &selected))
                {
                    variant = enumValue;
                    edited  = true;
                }
            }
            ImGui::EndCombo();
        }
    }
    else
    {
        ImGui::Text("Could not draw variant of type {%s}",
                    variant.get_type().get_name().data());
    }

    return edited;
}

void PropertyDrawer::DrawReadonlyVariant(rttr::variant variant, bool disabled)
{
    ImGui::BeginDisabled(disabled);

    const bool isWrapper = variant.get_type().is_wrapper();

    while (variant.get_type().is_wrapper())
    {
        variant = variant.extract_wrapped_value();
    }

    if (variant.is_type<sbk_id>() && isWrapper)
    {
        sbk::core::database_ptr<sbk::core::database_object> object(
            variant.convert<sbk_id>());

        if (object.lookup())
        {
            ImGui::TextUnformatted(object->get_database_name().data());
        }
        else if (object.hasId())
        {
            ImGui::Text("object Unloaded {%s}", rttr::variant(object.id()).to_string().c_str());
        }
        else
        {
            ImGui::TextUnformatted("NULL");
        }
    }
    else if (variant.can_convert(rttr::type::get<std::string>()))
    {
        ImGui::TextUnformatted(variant.to_string().c_str());
    }
    else if (variant.can_convert(rttr::type::get<std::string_view>()))
    {
        ImGui::TextUnformatted(variant.convert<std::string_view>().data());
    }
    else if (variant.get_type() == rttr::type::get<std::filesystem::path>())
    {
        ImGui::TextUnformatted(variant.convert<std::filesystem::path>().string().c_str());
    }
    else
    {
        ImGui::BeginDisabled();

        DrawVariant(variant, variant.get_type().get_name());

        ImGui::EndDisabled();
    }

    ImGui::EndDisabled();
}

bool PropertyDrawer::DrawFloat(float& value,
                               rttr::string_view name,
                               std::pair<float, float>& minMax)
{
    if (minMax.first == minMax.second)
    {
        minMax = std::pair<float, float>(0.0f, 1.0f);
    }

    return ImGui::SliderFloat(name.data(), &value, minMax.first, minMax.second);
}

bool PropertyDrawer::DrawInt(int& value,
                             rttr::string_view name,
                             std::pair<int, int>& minMax)
{
    if (minMax.first == minMax.second)
    {
        minMax = std::pair<int, int>(0, 1);
    }

    return ImGui::SliderInt(name.data(), &value, minMax.first, minMax.second);
}

bool PropertyDrawer::DrawBool(bool& value, rttr::string_view name)
{
    return ImGui::Checkbox(name.data(), &value);
}

bool PropertyDrawer::DrawMemberObject(rttr::variant& value,
                                      rttr::string_view name)
{
    bool edited = false;

    ImGui::PushID(name.data());

    ImGui::TextUnformatted(name.data());

    int index = 0;

    for (rttr::property childProperty : value.get_type().get_properties())
    {
        ImGui::PushID(index++);
        if (DrawProperty(childProperty, rttr::instance(value)))
        {
            edited = true;
        }
        ImGui::PopID();
    }

    ImGui::PopID();

    return edited;
}

bool PropertyDrawer::DrawSequentialContainer(
    rttr::variant_sequential_view& view, rttr::string_view name)
{
    bool edited = false;

    if (view.is_empty())
    {
        ImGui::TextUnformatted("Empty");
    }

    ImGui::SameLine();

    ImGui::Text("%lu elements", view.get_size());

    ImGui::SameLine();

    if (ImGui::Button("+"))
    {
        const rttr::type type = view.get_value_type();

        rttr::variant createdDefault;

        if (type.is_wrapper())
        {
            assert(type.get_wrapped_type() == rttr::type::get<sbk_id>());
            createdDefault = (sbk_id)0;
            const bool converted = createdDefault.convert(type);
            assert(converted);
        }
        else
        {
            assert(type.is_class());
            createdDefault = type.create_default();
        }

        assert(createdDefault.is_valid());

        rttr::variant_sequential_view::const_iterator insertedIterator =
            view.insert(view.begin() + view.get_size(), createdDefault);

        edited = insertedIterator != view.end();
        assert(edited);
    }

    if (view.get_size())
    {
        int index = 0;

        for (rttr::variant_sequential_view::const_iterator iterator =
                 view.begin();
             iterator != view.end(); ++iterator)
        {
            ImGui::PushID(index);

            rttr::variant value = iterator.get_data().extract_wrapped_value();

            if (DrawVariant(value, std::to_string(index).data()))
            {
                view.set_value(index, value);
                edited = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("X"))
            {
                iterator = view.erase(iterator);
                edited = true;
            }

            ImGui::PopID();

            ++index;

            if (iterator == view.end())
            {
                break;
            }
        }
    }

    return edited;
}

bool PropertyDrawer::DrawAssociateContainer(
    rttr::variant_associative_view& view, rttr::string_view name)
{
    bool edited = false;

    if (ImGui::Button("+"))
    {
        const rttr::type keyType   = view.get_key_type();
        const rttr::type valueType = view.get_value_type();

        if (view.is_key_only_type())
        {
            std::pair<rttr::variant_associative_view::const_iterator, bool>
                insertedIterator = view.insert(keyType.create_default());
            edited               = insertedIterator.second;
        }
        else
        {
            rttr::variant key;
            rttr::variant value;

            /**
             * @todo Can we construct basic types without any manual work?
             */
            if (keyType.is_wrapper() &&
                keyType.get_wrapped_type() == rttr::type::get<sbk_id>())
            {
                sbk_id id = 0;
                key      = id;
                key.convert(keyType);
            }

            if (valueType.is_wrapper() &&
                valueType.get_wrapped_type() == rttr::type::get<sbk_id>())
            {
                sbk_id id = 0;
                value    = id;
                value.convert(valueType);
            }

            assert(key.is_valid());
            assert(value.is_valid());

            std::pair<rttr::variant_associative_view::const_iterator, bool>
                insertedIterator = view.insert(key, value);
            edited               = insertedIterator.second;
        }

        return edited;  // exiting early to ensure the parent property is
                        // updated
    }

    if (view.get_size())
    {
        if (ImGui::CollapsingHeader(name.data(),
                                    ImGuiTreeNodeFlags_DefaultOpen))
        {
            int index = 0;

            if (ImGui::BeginTable(name.data(), 2, ImGuiTableFlags_Resizable))
            {
                for (rttr::variant_associative_view::const_iterator iter =
                         view.begin();
                     iter != view.end(); ++iter)
                {
                    ImGui::PushID(index++);

                    rttr::variant key = iter.get_key().extract_wrapped_value();
                    rttr::variant value =
                        iter.get_value().extract_wrapped_value();

                    if (view.is_key_only_type())
                    {
                        ImGui::TableNextColumn();  // enter column 1
                        ImGui::TableNextColumn();  // enter column 2

                        edited =
                            DrawVariant(key, std::to_string(index).c_str());
                    }
                    else
                    {
                        ImGui::TableNextColumn();
                        bool keyEdited =
                            DrawVariant(key, std::to_string(index).c_str());
                        ImGui::TableNextColumn();
                        bool valueEdited = DrawVariant(value, "Value");

                        if (keyEdited || valueEdited)
                        {
                            edited = true;

                            const size_t removed = view.erase(key);

                            if (removed > 0)
                            {
                                std::pair<rttr::variant_associative_view::
                                              const_iterator,
                                          bool>
                                    insertedIter = view.insert(key, value);

                                if (insertedIter.second)
                                {
                                    iter = insertedIter.first;
                                }
                                else
                                {
                                    ImGui::PopID();
                                    break;
                                }
                            }
                        }
                    }

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextUnformatted("Empty");
    }

    return edited;
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) &&
        ImGui::BeginTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool PropertyDrawer::DrawPayloadDrop(rttr::variant& value,
                                     const rttr::variant& payloadString)
{
    bool edited = false;

    ImGui::BeginGroup();

    DrawReadonlyVariant(value, false);

    if (ImGui::BeginDragDropTarget())
    {
        std::string payloadStringString = payloadString.to_string();

        // Allow all
        if (payloadStringString == sbk::editor::PayloadObject)
        {
            if (const ImGuiPayload* const payload = ImGui::GetDragDropPayload())
            {
                payloadStringString = payload->DataType;
            }
        }

        if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(payloadStringString.c_str()))
        {
            const rttr::type valueType = value.get_type();
            rttr::variant data;

            if (valueType == rttr::type::get<std::string>())
            {
                char* payloadCharString = static_cast<char*>(payload->Data);
                data                    = std::string(payloadCharString);
            }
            else if (valueType == rttr::type::get<sbk_id>() ||
                     (valueType.is_wrapper() &&
                      valueType.get_wrapped_type() == rttr::type::get<sbk_id>()))
            {
                sbk_id* payloadID = static_cast<sbk_id*>(payload->Data);
                data             = *payloadID;

                if (valueType.is_wrapper())
                {
                    data.convert(valueType);
                }
            }

            // Hack for child pointers
            // When setting variants, the value is completely overriden.
            // There is no chance for child pointers to retain its parent value and check the child.
            // Therefore, we have to do this ourselves here.
            if (valueType.is_wrapper() && std::string(valueType.get_name().data()).find("ChildPtr") != std::string::npos)
            {
                bool convertSuccess = false;
                sbk::core::ChildPtr<sbk::core::database_object> dataAsChildPtr = data.convert<sbk::core::ChildPtr<sbk::core::database_object>>(&convertSuccess);

                if (convertSuccess)
                {
                    sbk::core::ChildPtr<sbk::core::database_object> valueAsChildPtr = value.convert<sbk::core::ChildPtr<sbk::core::database_object>>(&convertSuccess);

                    if (convertSuccess)
                    {
                        sbk_id currentID = valueAsChildPtr.id();

                        valueAsChildPtr = dataAsChildPtr;

                        if (currentID != valueAsChildPtr.id())
                        {
                            value = valueAsChildPtr;
                            value.convert(valueType);
                            assert(value.is_valid());
                            edited = value.is_valid();
                        }
                    }
                }
            }
            else
            {
                value.swap(data);
                edited = true;
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::EndGroup();

    return edited;
}

bool PropertyDrawer::DrawPayloadDrop(rttr::property property,
                                     rttr::instance object,
                                     const rttr::variant& payloadString)
{
    bool edited = false;

    const rttr::type propertyType = property.get_type();

    ImGui::BeginGroup();

    rttr::variant value = property.get_value(object);

    DrawReadonlyVariant(value, false);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* const payload =
                ImGui::AcceptDragDropPayload(payloadString.to_string().c_str()))
        {
            rttr::variant data;

            if (propertyType == rttr::type::get<std::string>())
            {
                char* payloadCharString = static_cast<char*>(payload->Data);
                data                    = std::string(payloadCharString);
            }
            else if (propertyType == rttr::type::get<sbk_id>() ||
                     (propertyType.is_wrapper() &&
                      propertyType.get_wrapped_type() ==
                          rttr::type::get<sbk_id>()))
            {
                sbk_id* payloadID = static_cast<sbk_id*>(payload->Data);
                data             = *payloadID;

                if (propertyType.is_wrapper())
                {
                    data.convert(propertyType);
                }
            }

            edited = property.set_value(object, data);
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::EndGroup();

    return edited;
}
