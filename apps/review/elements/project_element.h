#include "pch.h"

#include "gluten/elements/element.h"

class project_element : public gluten::element
{
public:
    project_element() : gluten::element(anchor_preset::stretch_full) {}
    project_element(const std::string& projectName,
                    const std::string& projectDescription,
                    int openReviews,
                    int closedReviews,
                    int archivedReviews)
        : gluten::element(anchor_preset::stretch_full), m_projectName(projectName), m_projectDescription(projectDescription), m_openReviews(openReviews), m_closedReviews(closedReviews), m_archivedReviews(archivedReviews)
    {
        set_element_background_color(gluten::theme::field03);
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;

private:
    std::string m_projectName;
    std::string m_projectDescription;
    int m_openReviews     = 0;
    int m_closedReviews   = 0;
    int m_archivedReviews = 0;
};