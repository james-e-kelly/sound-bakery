#include "PropertyDrawer.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/node/node.h"

#include "imgui.h"

void PropertyDrawer::DrawObject(rttr::type type, rttr::instance instance)
{
	ImGui::PushID(type.get_name().data());

	int index = 0;

	for (rttr::property property : type.get_properties())
	{
		DrawProperty(property, instance);
	}

	ImGui::PopID();
}

bool PropertyDrawer::DrawProperty(rttr::property property, rttr::instance instance)
{
	bool edited = false;

	bool readonly = property.is_readonly();

	if (!readonly)
	{
		readonly = property.get_metadata(SB::Editor::METADATA_KEY::Readonly).to_bool();
	}

	ImGui::PushID(property.get_name().data());

	rttr::variant propertyValue = property.get_value(instance);

	ImGui::Text("%s: ", property.get_name().data());
	ImGui::SameLine();

	if (readonly)
	{
		DrawReadonlyVariant(propertyValue);
	}
	else if (const rttr::variant payloadString = property.get_metadata(SB::Editor::METADATA_KEY::Payload))
	{
		edited = DrawPayloadDrop(property, instance, payloadString);
	}
	else if (DrawVariant(propertyValue, property.get_name(), property.get_metadata(SB::Editor::METADATA_KEY::MinMax)))
	{
		edited = property.set_value(instance, propertyValue);
	}

	ImGui::PopID();

	return edited;
}

bool PropertyDrawer::DrawVariant(rttr::variant& variant, rttr::string_view name, rttr::variant minMax)
{
	const rttr::type type = variant.get_type();
	bool edited = false;

	assert(type != rttr::type::get<rttr::variant>());

	if (type.is_arithmetic())
	{
		if (type == rttr::type::get<float>())
		{
			float value = variant.to_float();

			std::pair<float, float> floatMinMax = minMax.convert<std::pair<float, float>>();

			if (DrawFloat(value, name, floatMinMax))
			{
				variant = value;
				edited = true;
			}
		}
		else if (type == rttr::type::get<int>())
		{
			int value = variant.to_int();

			std::pair<int, int> intMinMax = minMax.convert<std::pair<int, int>>();

			if (DrawInt(value, name, intMinMax))
			{
				variant = value;
				edited = true;
			}
		}
		else if (type == rttr::type::get<bool>())
		{
			bool value = variant.to_bool();

			if (DrawBool(value, name))
			{
				variant = value;
				edited = true;
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
		SB_ID id = variant.extract_wrapped_value().convert<SB_ID>();
		rttr::type templateType = *type.get_template_arguments().begin();

		SB::Core::DatabasePtr<SB::Core::Object> objectPtr(id);

		if (objectPtr.lookup())
		{
			DrawObject(objectPtr->getType(), rttr::instance(objectPtr.raw()));
		}
		else if (objectPtr.hasId())
		{
			ImGui::TextUnformatted("Object Unloaded");
		}
		else
		{
			ImGui::TextUnformatted("NULL");
		}
	}
	else if (type.is_pointer())
	{
		DrawReadonlyVariant(variant);
	}
	else if (type.is_wrapper())
	{
		DrawReadonlyVariant(variant);
	}
	else if (type == rttr::type::get<std::string>() || type == rttr::type::get<std::string_view>())
	{
		DrawReadonlyVariant(variant);
	}
	else if (type.is_class())
	{
		if (type == rttr::type::get<SB::Engine::EffectParameterDescription>())
		{
			SB::Engine::EffectParameterDescription effectParamterDescription = variant.convert<SB::Engine::EffectParameterDescription>();

			switch (effectParamterDescription.m_parameter.m_type)
			{
			case SC_DSP_PARAMETER_TYPE_FLOAT:
				SB::Editor::MinMax minMax(effectParamterDescription.m_parameter.m_float.m_min, effectParamterDescription.m_parameter.m_float.m_max);
				edited = DrawFloat(effectParamterDescription.m_parameter.m_float.m_value, effectParamterDescription.m_parameter.m_name, minMax);
				break;
			}

			if (edited)
			{
				variant = effectParamterDescription;
			}
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
				ImGui::TextUnformatted("Object doesn't have properties");
			}
		}
	}
	else if (type.is_enumeration())
	{
		const rttr::enumeration enumeration = type.get_enumeration();

		const rttr::string_view previewName = enumeration.value_to_name(variant);

		if (ImGui::BeginCombo(enumeration.get_name().data(), previewName.data()))
		{
			for (const rttr::string_view& enumValueName : enumeration.get_names())
			{
				rttr::variant enumValue = enumeration.name_to_value(enumValueName);
				bool selected = enumValue == variant;
				if (ImGui::Selectable(enumValueName.data(), &selected))
				{
					variant = enumValue;
					edited = true;
				}
			}
			ImGui::EndCombo();
		}
	}
	else
	{
		ImGui::Text("Could not variant of type {%s}", variant.get_type().get_name().data());
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

	if (variant.is_type<SB_ID>() && isWrapper)
	{
		SB::Core::DatabasePtr<SB::Core::DatabaseObject> object(variant.convert<SB_ID>());

		if (object.lookup())
		{
			ImGui::TextUnformatted(object->getDatabaseName().data());
		}
		else if (object.hasId())
		{
			ImGui::TextUnformatted("Object Unloaded");
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
	else
	{
		ImGui::BeginDisabled();

		DrawVariant(variant, variant.get_type().get_name());

		ImGui::EndDisabled();
	}

	ImGui::EndDisabled();
}

bool PropertyDrawer::DrawFloat(float& value, rttr::string_view name, std::pair<float, float>& minMax)
{
	if (minMax.first == minMax.second)
	{
		minMax = std::pair<float, float>(0.0f, 1.0f);
	}

	return ImGui::SliderFloat(name.data(), &value, minMax.first, minMax.second);
}

bool PropertyDrawer::DrawInt(int& value, rttr::string_view name, std::pair<int, int>& minMax)
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

bool PropertyDrawer::DrawMemberObject(rttr::variant& value, rttr::string_view name)
{
	bool edited = false;

	ImGui::PushID(name.data());

	ImGui::TextUnformatted(name.data());

	int index = 0;

	for (rttr::property childProperty : value.get_type().get_properties())
	{
		ImGui::PushID(index++);
		if (DrawProperty(childProperty, value))
		{
			edited = true;
		}
		ImGui::PopID();
	}
	
	ImGui::PopID();

	return edited;
}

bool PropertyDrawer::DrawSequentialContainer(rttr::variant_sequential_view& view, rttr::string_view name)
{
	bool edited = false;

	if (view.is_empty())
	{
		ImGui::TextUnformatted("Empty");
	}
	
	ImGui::SameLine();

	if (ImGui::Button("+"))
	{
		const rttr::type type = view.get_value_type();

		rttr::variant_sequential_view::const_iterator insertedIterator = view.insert(view.begin() + view.get_size(), type.create());

		edited = insertedIterator != view.end();
	}

	if (view.get_size())
	{
		int index = 0;

		for (rttr::variant_sequential_view::const_iterator iterator = view.begin(); iterator != view.end(); ++iterator)
		{
			ImGui::PushID(index);

			rttr::variant value = iterator.get_data().extract_wrapped_value();

			if (ImGui::Button("X"))
			{
				view.erase(iterator);
				edited = true;
				break;
			}

			if (DrawVariant(value, std::to_string(index).data()))
			{
				view.set_value(index, value);
				edited = true;
			}

			ImGui::PopID();

			++index;
		}
	}

	return edited;
}

bool PropertyDrawer::DrawAssociateContainer(rttr::variant_associative_view& view, rttr::string_view name)
{
	bool edited = false;

	if (ImGui::Button("+"))
	{
		const rttr::type keyType = view.get_key_type();
		const rttr::type valueType = view.get_value_type();

		if (view.is_key_only_type())
		{
			std::pair<rttr::variant_associative_view::const_iterator, bool> insertedIterator = view.insert(keyType.create());
			edited = insertedIterator.second;
		}
		else
		{
			rttr::variant key;
			rttr::variant value;

			/**
			 * @todo Can we construct basic types without any manual work?
			 */
			if (keyType.is_wrapper() && keyType.get_wrapped_type() == rttr::type::get<SB_ID>())
			{
				SB_ID id = 0;
				key = id;
				key.convert(keyType);
			}

			if (valueType.is_wrapper() && valueType.get_wrapped_type() == rttr::type::get<SB_ID>())
			{
				SB_ID id = 0;
				value = id;
				value.convert(valueType);
			}

			assert(key.is_valid());
			assert(value.is_valid());

			std::pair<rttr::variant_associative_view::const_iterator, bool> insertedIterator = view.insert(key, value);
			edited = insertedIterator.second;
		}

		return edited;	// exiting early to ensure the parent property is updated
	}

	if (view.get_size())
	{
		if (ImGui::CollapsingHeader(name.data(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			int index = 0;

			for (const rttr::variant& item : view)
			{
				ImGui::PushID(index++);

				const std::pair<rttr::variant, rttr::variant> keyValuePair = item.convert<std::pair<rttr::variant, rttr::variant>>();

				rttr::variant key = keyValuePair.first;
				rttr::variant value = view.is_key_only_type() ? keyValuePair.first : keyValuePair.second;

				if (view.is_key_only_type())
				{
					ImGui::Bullet();

					edited = DrawVariant(value, std::to_string(index).c_str());
				}
				else
				{
					key = key.extract_wrapped_value();
					value = value.extract_wrapped_value();

					if (ImGui::BeginTable(name.data(), 2))
					{
						ImGui::TableNextColumn();
						bool keyEdited = DrawVariant(key, std::to_string(index).c_str());
						ImGui::TableNextColumn();
						bool valueEdited = DrawVariant(value, "Value");
						ImGui::EndTable();

						if (keyEdited || valueEdited)
						{
							edited = true;
							auto iter = view.insert(key, value);
							assert(iter.second);
						}
					}
				}

				ImGui::PopID();
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
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

bool PropertyDrawer::DrawPayloadDrop(rttr::property property, rttr::instance object, const rttr::variant& payloadString)
{
	bool edited = false;

	const rttr::type propertyType = property.get_type();

	ImGui::BeginGroup();

	rttr::variant value = property.get_value(object);
	
	DrawReadonlyVariant(value, false);

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* const payload = ImGui::AcceptDragDropPayload(payloadString.to_string().c_str()))
		{
			rttr::variant data;

			if (propertyType == rttr::type::get<std::string>())
			{
				char* payloadCharString = static_cast<char*>(payload->Data);
				data = std::string(payloadCharString);
			}
			else if (propertyType == rttr::type::get<SB_ID>() || (propertyType.is_wrapper() && propertyType.get_wrapped_type() == rttr::type::get<SB_ID>()))
			{
				SB_ID* payloadID = static_cast<SB_ID*>(payload->Data);
				data = *payloadID;

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