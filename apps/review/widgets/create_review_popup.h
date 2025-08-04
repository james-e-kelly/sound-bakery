#include "pch.h"

#include "data/review_data.h"

class create_review_popup : public gluten::popup_widget
{
    WIDGET_CONSTRUCT_PARENT(create_review_popup, "Create Review", gluten::popup_widget)

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char reviewNameBuffer[textBufferSize]        = {0};
    char reviewDescriptionBuffer[textBufferSize] = {0};
    char reviewUrlBuffer[textBufferSize] = {0};
    new_frontend_review_data m_reviewData;

    concurrencpp::result<void> m_asyncCreateReviewResult;
};