#pragma once

#include "IconsLucide.h"
#include "imspinner.h"
#include "gluten/app/app.h"
#include "gluten/data/data_source.h"
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
#include <magic_enum/magic_enum.hpp>

using project_id_type = unsigned int;

enum review_app_version
{
    review_app_version_start = 0,
    review_app_save_selected_project,
    review_app_user_tokens,

    // Add versions above this //
    review_app_version_end,
    review_app_version_current = review_app_version_end - 1
};