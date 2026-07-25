#include "pch.h"

#include "data/review_data.h"
#include "misc/edit_reviewers.h"

class create_review_popup : public gluten::popup_widget, protected edit_reviewers
{
public:
    create_review_popup(gluten::widget* widgetParent)
        : gluten::popup_widget(widgetParent, "Create Review"),
          edit_reviewers()
    {
    }

    create_review_popup(gluten::widget* widgetParent, int64_t existingReviewId)
        : gluten::popup_widget(widgetParent, "Create New Version"), m_existingReviewId(existingReviewId), edit_reviewers()
    {
    }

    MulticastDelegate<> onCompleteDelegate;

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char reviewNameBuffer[textBufferSize]        = {0};
    char reviewDescriptionBuffer[textBufferSize] = {0};
    char reviewUrlBuffer[textBufferSize]         = {0};
    new_frontend_review_data m_reviewData;

    std::optional<int64_t> m_existingReviewId;

    concurrencpp::result<void> m_asyncCreateReviewResult;
};