#include "pch.h"

class create_project_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(create_project_popup, "Create Project", gluten::popup_widget)

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char projectNameBuffer[textBufferSize]        = {0};
    char projectDescriptionBuffer[textBufferSize] = {0};
};