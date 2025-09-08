#pragma once

#include "pch.h"

struct base_comment_data
{
    int64_t m_userId   = 0;
    int64_t m_reviewId = 0;
    int64_t m_fileId   = 0;
    std::string m_comment;
    double m_timeStart = -1.0;
    double m_timeEnd   = -1.0;
};

struct comment_data : public base_comment_data
{
    int64_t m_commentId = 0;    //< Adds in the row id of this comment
    std::string m_timestamp;    //< Adds a timestamp of when the comment was added to the database
};

struct new_comment_data : public base_comment_data
{
    new_comment_data();
};