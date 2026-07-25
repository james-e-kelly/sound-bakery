#pragma once

#include "pch.h"

struct base_comment_data
{
    database_id m_userId   = 0;
    database_id m_reviewId = 0;
    database_id m_fileId   = 0;
    std::string m_comment;
    double m_timeStart = -1.0;
    double m_timeEnd   = -1.0;

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& BOOST_SERIALIZATION_NVP(m_userId);
        archive& BOOST_SERIALIZATION_NVP(m_reviewId);
        archive& BOOST_SERIALIZATION_NVP(m_fileId);
        archive& BOOST_SERIALIZATION_NVP(m_comment);
        archive& BOOST_SERIALIZATION_NVP(m_timeStart);
        archive& BOOST_SERIALIZATION_NVP(m_timeEnd);
    }
};

struct comment_data : public base_comment_data
{
    database_id m_commentId = 0;  //< Adds in the row id of this comment
    std::string m_timestamp;      //< Adds a timestamp of when the comment was added to the database

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_comment_data);
        archive& BOOST_SERIALIZATION_NVP(m_commentId);
        archive& BOOST_SERIALIZATION_NVP(m_timestamp);
    }
};

struct new_comment_data : public base_comment_data
{
    new_comment_data();

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive& BOOST_SERIALIZATION_BASE_OBJECT_NVP(base_comment_data);
    }
};