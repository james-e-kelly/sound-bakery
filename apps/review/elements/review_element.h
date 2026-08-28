#pragma once

#include "pch.h"

#include "data/review_data.h"
#include "list_element.h"

class review_element : public list_element
{
public:
    review_element() = delete;
    review_element(const review_data& review)
        : list_element
        (
            review.m_reviewName, 
            review.m_reviewDescription, 
            review.m_reviewStatus == review_status::open ? ICON_LC_EYE : review.m_reviewStatus == review_status::closed ? ICON_LC_CHECK_LINE : ICON_LC_ARCHIVE
        ),
          m_reviewId(review.m_reviewId)
    {

    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    database_id m_reviewId{};
};