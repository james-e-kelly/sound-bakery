#pragma once

#include "IconsLucide.h"
#include "imspinner.h"
#include "gluten/app/app.h"
#include "gluten/data/data_source.h"
#include "gluten/data/data_cache.h"
#include "gluten/elements/icon_button.h"
#include "gluten/elements/collapsing_header.h"
#include "gluten/elements/loading_spinner.h"
#include "gluten/elements/layouts/layout.h"
#include "gluten/theme/carbon_theme_g100.h"
#include "gluten/managers/manager.h"
#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "gluten/utils/imgui_util_structures.h"
#include "gluten/widgets/root_widget.h"
#include "gluten/widgets/window_widget.h"
#include "gluten/widgets/popup_widget.h"
#include "httplib.h"
#include "tl/expected.hpp"

#include "boost/serialization/version.hpp"
#include "boost/serialization/vector.hpp"
#include "boost/serialization/map.hpp"
#include <magic_enum/magic_enum.hpp>

using database_id = int64_t;

enum review_app_version
{
    review_app_version_start = 0,
    review_app_save_selected_project,
    review_app_user_tokens,
    review_app_first_settings,
    review_app_stored_expiry_time,

    // Add versions above this //
    review_app_version_end,
    review_app_version_current = review_app_version_end - 1
};

struct review_app_api
{
    static inline std::string getworkspaceName = "/get-workspace-name";
    static inline std::string getAllProjects = "/get-all-projects";
    static inline std::string getAllReviews = "/get-all-reviews";
    static inline std::string getReviewVote = "/get-review-vote";
};

struct review_app_parameters
{
    static inline std::string projectId = "projectId";
    static inline std::string reviewId = "reviewId";
    static inline std::string userId = "userId";
};