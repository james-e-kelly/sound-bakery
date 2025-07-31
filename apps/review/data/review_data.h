#pragma once

#include "pch.h"

#include "data/user_data.h"

enum class review_phase
{
    temp,
    first_pass,
    iteration_pass,
    final_pass,
    num
};

auto get_review_phase_string(review_phase phase) -> std::string;

enum class review_quality
{
    c,
    b,
    a,
    a_plus,
    a_plus_plus,
    num
};

auto get_review_quality_string(review_quality quality) -> std::string;

enum class review_status
{
    open,
    closed,
    archived
};

auto get_review_status_string(review_status status) -> std::string;

enum class review_vote
{
    no_vote,
    upvote,
    downvote    //< We have downvotes but try not to use or show them
};

struct versionable_review_asset
{
    std::string m_fileName;             //< Filename for displaying
    std::size_t m_currentAssetVersion;  //< The current version of the asset. Also the number of versions this file has
    std::map<std::size_t, std::filesystem::path> m_versionsToRelativeFiles; //< Relative paths of each version of the asset

    template <class archive_class>
    auto serialize(archive_class& archive, const unsigned int version) -> void
    {
        archive & boost::serialization::make_nvp("file_name", m_fileName);
        archive & boost::serialization::make_nvp("current_version", m_currentAssetVersion);
        archive & boost::serialization::make_nvp("file_version", m_versionsToRelativeFiles);
    }
};

struct review_data
{
    int64_t m_reviewId = 0;
    std::string m_reviewName;
    std::string m_reviewTaskUrl;
    std::string m_reviewDescription;
    review_phase m_reviewPhase = review_phase::first_pass;
    review_quality m_reviewQuality = review_quality::c;
    review_status m_reviewStatus   = review_status::open;
    std::vector<versionable_review_asset> m_relativeContextFiles;   //< Context files are also versionable in case new context is required
    std::vector<versionable_review_asset> m_reviewAssets;
    std::vector<int64_t> m_reviewUserIds;

    bool operator<(const review_data& review) const
    {
        return m_reviewId < review.m_reviewId;
    }
};

struct new_review_data
{
    std::string m_reviewName;
    std::string m_reviewTaskUrl;
    std::string m_reviewDescription;
    review_phase m_reviewPhase = review_phase::first_pass;
    review_quality m_reviewQuality = review_quality::c;

    std::unordered_set<std::filesystem::path> m_absoluteContextFiles;  //< Absolute files to copy into the review folder
    std::unordered_set<std::filesystem::path> m_absoluteReviewFiles;   //< Absolute files to copy into the review folder
};

/**
 * @brief User data plus their vote on this review
 */
struct reviewer_data : public user_data
{
    reviewer_data() = default;

    reviewer_data(const user_data& userData)
        : user_data(userData),
          m_vote(review_vote::no_vote) {}

    reviewer_data(const user_data& userData, review_vote vote)
        : user_data(userData),
          m_vote(vote) {}

    review_vote m_vote = review_vote::no_vote;
};

BOOST_CLASS_VERSION(versionable_review_asset, review_app_version_current)
BOOST_CLASS_VERSION(review_data, review_app_version_current)
BOOST_CLASS_VERSION(new_review_data, review_app_version_current)