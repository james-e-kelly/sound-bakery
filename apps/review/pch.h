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
#include "boost/serialization/array.hpp"
#include "platform_folders.h"
#include <magic_enum/magic_enum.hpp>

auto http_result_okay(const httplib::Result& result) -> bool;

namespace review_app_serialization
{
    template <typename T>
    auto deserialize_from_xml(const std::string& body) -> T
    {
        T data;
        std::istringstream inputStream(body);
        boost::archive::xml_iarchive archive(inputStream);
        archive & BOOST_SERIALIZATION_NVP(data);
        return data;
    }

    template <typename T>
    auto serialize_to_xml(const T& data) -> std::string
    {
        std::ostringstream outputStream;
        {
            boost::archive::xml_oarchive archive(outputStream);
            archive & BOOST_SERIALIZATION_NVP(data);
        }
        return outputStream.str();
    }
}

using database_id = int64_t;

enum review_app_version
{
    review_app_version_start = 0,
    review_app_save_selected_project,
    review_app_user_tokens,
    review_app_first_settings,
    review_app_stored_expiry_time,
    review_app_ip_address,

    // Add versions above this //
    review_app_version_end,
    review_app_version_current = review_app_version_end - 1
};

struct review_app_endpoints
{
    static inline std::string me = "/me";
    static inline std::string workspace = "/workspace";
    static inline std::string projects = "/projects";
    static inline std::string reviews = "/reviews";
    static inline std::string reviewVotes = "/votes";
    static inline std::string reviewUsers = "/review-users";
    static inline std::string users = "/users";
    static inline std::string comments = "/comments";
    static inline std::string activity = "/activity";
    static inline std::string login = "/login";
    static inline std::string files = "/files";
    static inline std::string queries = "/queries";
};

struct review_app_parameters
{
    static inline std::string projectId = "projectId";
    static inline std::string reviewId = "reviewId";
    static inline std::string userId = "userId";
    static inline std::string commentId = "commentId";

    static inline std::string name = "name";
    static inline std::string description = "description";
    static inline std::string data = "data";
    static inline std::string reviewFile = "review_file";
    static inline std::string contextFile = "context_file";
    static inline std::string file = "file";
    static inline std::string reviewStatus = "review_status";
    static inline std::string reviewVote = "review_vote";
    static inline std::string activityType = "activity_type";
    static inline std::string query = "query";
};

struct review_app_queries
{
    static inline std::string userTableEmptyQuery = "users_empty";
};