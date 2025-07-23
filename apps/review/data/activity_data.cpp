#include "activity_data.h"

auto get_activity_type_string(activity_type activityType) -> std::string
{
	std::string result;

	switch (activityType)
    {
        case activity_type::project_created:
            result = "Created a project";
            break;
        case activity_type::project_edited:
            result = "Edited a project";
            break;
        case activity_type::review_created:
            result = "Created a review";
            break;
        case activity_type::review_edited:
            result = "Edited a review";
            break;
        case activity_type::review_files_edited:
            result = "Edited the files in a review";
            break;
        case activity_type::comment_added:
            result = "Added a comment";
            break;
        case activity_type::comment_edited:
            result = "Edited a comment";
            break;
        default:
            break;
    }

    return result;
}