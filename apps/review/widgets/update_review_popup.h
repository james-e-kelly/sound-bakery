#include "pch.h"

#include "data/review_data.h"

class update_review_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(update_review_popup, "Update Review", gluten::popup_widget)

public:
    auto set_review_data(const review_data& reviewData) -> void;

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char reviewNameBuffer[textBufferSize]        = {0};
    char reviewDescriptionBuffer[textBufferSize] = {0};
    char reviewUrlBuffer[textBufferSize] = {0};
    review_data m_reviewData;
};