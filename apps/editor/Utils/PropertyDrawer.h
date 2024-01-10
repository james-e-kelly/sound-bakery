#pragma once

#include <rttr/type>

// Draws RTTR properties to ImGui
class PropertyDrawer final
{
public:
	static void DrawObject(rttr::type type, rttr::instance instance);

	static bool DrawProperty(rttr::property property, rttr::instance instance);

	static bool DrawVariant(rttr::variant& variant, rttr::string_view name, rttr::variant minMax = rttr::variant());
	static void DrawReadonlyVariant(rttr::variant variant, bool disabled = true);

public:
	static bool DrawFloat(float& value, rttr::string_view name, std::pair<float, float>& minMax);
	static bool DrawInt(int& value, rttr::string_view name, std::pair<int, int>& minMax);
	static bool DrawBool(bool& value, rttr::string_view name);

	static bool DrawMemberObject(rttr::variant& value, rttr::string_view name);

	static bool DrawSequentialContainer(rttr::variant_sequential_view& view, rttr::string_view name);
	static bool DrawAssociateContainer(rttr::variant_associative_view& view, rttr::string_view name);

private:
	static bool DrawPayloadDrop(rttr::variant& value, const rttr::variant& payloadString);
	static bool DrawPayloadDrop(rttr::property property, rttr::instance object, const rttr::variant& payloadString);
};

