#pragma once

#include "pch.h"

enum class review_phase
{
    temp,
    first_pass,
    iteration_pass,
    final_pass,
    num
};

enum class review_quality
{
    unknown,
    c,
    b,
    a,
    a_plus,
    a_plus_plus,
    num
};

enum class review_status
{
    open,
    closed,
    archived
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
    review_quality m_reviewQuality = review_quality::unknown;
    review_status m_reviewStatus   = review_status::open;
    std::vector<versionable_review_asset> m_relativeContextFiles;   //< Context files are also versionable in case new context is required
    std::vector<versionable_review_asset> m_reviewAssets;
};

struct new_review_data
{
    std::string m_reviewName;
    std::string m_reviewTaskUrl;
    std::string m_reviewDescription;
    review_phase m_reviewPhase = review_phase::first_pass;
    review_quality m_reviewQuality = review_quality::unknown;

    std::unordered_set<std::filesystem::path> m_absoluteContextFiles;  //< Absolute files to copy into the review folder
    std::unordered_set<std::filesystem::path> m_absoluteReviewFiles;   //< Absolute files to copy into the review folder
};

BOOST_CLASS_VERSION(versionable_review_asset, review_app_version_current)
BOOST_CLASS_VERSION(review_data, review_app_version_current)
BOOST_CLASS_VERSION(new_review_data, review_app_version_current)