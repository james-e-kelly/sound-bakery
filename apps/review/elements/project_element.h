#pragma once

#include "pch.h"

#include "list_element.h"

class project_element : public list_element
{
public:
    project_element(const std::string& projectName,
                    const std::string& projectDescription,
                    int openReviews,
                    int closedReviews,
                    int archivedReviews)
        : list_element
        (
              projectName,
        projectDescription,
              openReviews > 0 ? fmt::format("{} {}", openReviews, ICON_LC_PENCIL) : ""
        )
    {
    }

protected:
    auto render_element(const gluten::element_render_info& renderInfo) -> bool override;
};