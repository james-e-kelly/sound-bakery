#pragma once

#include "gluten/elements/element.h"
#include "imgui.h"

namespace gluten
{
    template <typename T>
    concept valid_slider = requires(T a) 
    { 
        std::same_as<T, float> || std::same_as<T, int> || std::same_as<T, double>;
    };

    template <valid_slider T>
	class slider : public element
	{
    public:
        slider() = delete;
        slider(const char* label, T* value, T min, T max) : element(anchor_preset::stretch_full), m_label(label), m_value(value), m_min(min), m_max(max) {}

        bool render_element(const ImRect& elementBox) override 
        { 
            ImGui::SetNextItemWidth(elementBox.GetSize().x);
            return render_slider<T>(elementBox);
        }

        template<valid_slider U>
        bool render_slider(const ImRect& elementBox) {}

        template <>
        bool render_slider<float>(const ImRect& elementBox)
        {
            return ImGui::DragFloat(m_label, m_value, 1.0f, m_min, m_max);
        }

        template <>
        bool render_slider<double>(const ImRect& elementBox)
        {
            return ImGui::DragScalar(m_label, ImGuiDataType_Double, m_value, 1.0f, &m_min, &m_max);
        }

        template <>
        bool render_slider<int>(const ImRect& elementBox)
        {
            return ImGui::DragInt(m_label, m_value, 1.0f, m_min, m_max);
        }

	private:
        const char* m_label = nullptr;
        T* m_value;
        const T m_min;
        const T m_max;
	};
}