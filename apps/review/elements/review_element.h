#include "pch.h"

#include "gluten/elements/element.h"

#include "data/review_data.h"

class review_element : public gluten::element
{
public:
    review_element() = delete;
    review_element(const review_data& review)
        : gluten::element(anchor_preset::stretch_full),
        m_review(review)
    {
        set_element_background_color(gluten::theme::carbon_g100::field03);
    }

protected:
    auto render_element(const ImRect& elementRect) -> bool override;

private:
    const review_data& m_review;
};