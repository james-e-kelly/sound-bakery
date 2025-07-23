#pragma once

#include "pch.h"

struct comment_data
{
    int64_t m_commentId = 0;
    int64_t m_userId   = 0;
    int64_t m_reviewId = 0;
    int64_t m_fileId   = 0;
    std::string m_comment;
    std::string m_timestamp;
};