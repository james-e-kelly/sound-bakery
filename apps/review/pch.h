#pragma once

#include "gluten/app/app.h"
#include "gluten/managers/manager.h"
#include "gluten/subsystems/renderer_subsystem.h"
#include "gluten/subsystems/widget_subsystem.h"
#include "gluten/widgets/root_widget.h"

#include "boost/serialization/version.hpp"

using project_id_type = unsigned int;

enum review_app_version
{
    review_app_version_start = 0,
    review_app_save_selected_project,

    // Add versions above this //
    review_app_version_end,
    review_app_version_current = review_app_version_end - 1
};