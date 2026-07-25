#include "pch.h"

#include "data/review_data.h"
#include "gluten/elements/element.h"

class review_element : public gluten::element
{
public:
    review_element() = delete;
    review_element(const review_data& review)
        : gluten::element(anchor_preset::stretch_full),
          m_review(review)
    {
        set_element_background_color(gluten::theme::field03);
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    const review_data& m_review;
};