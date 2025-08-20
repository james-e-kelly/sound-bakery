#pragma once

#include "gluten/widgets/popup_widget.h"

class create_comment_popup : public gluten::popup_widget
{
public:
    create_comment_popup(gluten::widget* widgetParent, int64_t userId, int64_t reviewId, int64_t fileId, double videoPosition)
        : gluten::popup_widget(widgetParent, "Create Comment"), m_userId(userId), m_reviewId(reviewId), m_fileId(fileId), m_videoPosition(videoPosition)
    {
    }

protected:
    auto render_popup() -> void override;

private:
    static inline constexpr std::size_t textBufferSize = 512;

    char commentBuffer[textBufferSize] = {0};

    int64_t m_userId, m_reviewId, m_fileId;
    double m_videoPosition = 0.0;
};