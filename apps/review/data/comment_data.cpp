#include "comment_data.h"

#include "data/user_settings_data.h"
#include "gluten/data/data_source.h"

new_comment_data::new_comment_data()
{
    gluten::data_source<user_settings_data> userSettingsData;
    m_userId = userSettingsData->m_loggedInUser.m_userId;
}