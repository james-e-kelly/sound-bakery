#pragma once

#include <rttr/type>

// Draws RTTR properties to ImGui
class property_drawer final
{
public:
    static void draw_object(rttr::type type, rttr::instance instance);

    static bool draw_property(rttr::property property, rttr::instance instance);

    static bool draw_variant(rttr::variant& variant,
                            rttr::string_view name,
                            rttr::variant minMax = rttr::variant());

    static void draw_readonly_variant(rttr::variant variant,
                                    bool disabled = true);

public:
    static bool draw_float(float& value,
                          rttr::string_view name,
                          std::pair<float, float>& minMax);

    static bool draw_int(int& value,
                        rttr::string_view name,
                        std::pair<int, int>& minMax);

    static bool draw_bool(bool& value, rttr::string_view name);

    static bool draw_member_object(rttr::variant& value, rttr::string_view name);

    static bool draw_sequential_container(rttr::variant_sequential_view& view,
                                        rttr::string_view name);

    static bool draw_associate_container(rttr::variant_associative_view& view,
                                       rttr::string_view name);

private:
    static bool draw_payload_drop(rttr::variant& value,
                                const rttr::variant& payloadString);
    static bool draw_payload_drop(rttr::property property,
                                rttr::instance object,
                                const rttr::variant& payloadString);
};
