#include "pch.h"

#include "data/review_data.h"
#include "misc/edit_reviewers.h"

class create_review_popup : public gluten::popup_widget, protected edit_reviewers
{
public:
    create_review_popup(gluten::widget* widgetParent)
        : gluten::popup_widget(widgetParent, "Create Review"),
          edit_reviewers(0)
    {
    }

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