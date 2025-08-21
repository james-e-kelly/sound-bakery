#include "review_database.h"

#include "app/review_app.h"

#include "sodium.h"

namespace
{
    constexpr const char* g_createWorkspaceTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS workspaces (
            id INTEGER PRIMARY KEY AUTOINCREMENT, 
            name TEXT
        );
    )sql";

    constexpr const char* g_createProjectsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS projects (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )sql";

    constexpr const char* g_createUsersTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            display_name TEXT,
            title TEXT,
            email TEXT NOT NULL,
            password BLOB NOT NULL,
            salt BLOB NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            privilege INT NOT NULL DEFAULT 0,
            UNIQUE(email)
        );
    )sql";

    constexpr const char* g_createReviewsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS reviews (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            project_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            task_url TEXT,
            status INT DEFAULT 0,
            phase INT DEFAULT 0,
            quality INT DEFAULT 0,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(project_id) REFERENCES projects(id)
        );
    )sql";

    constexpr const char* g_createReviewAuthorsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS review_authors (
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (review_id, user_id),
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )sql";

    constexpr const char* g_createReviewersTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS reviewers (
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (review_id, user_id),
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )sql";

    constexpr const char* g_createReviewFilesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS review_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            file_name TEXT NOT NULL,            -- logical name (e.g. "footstep_sfx")
            file_type INT,                      -- context, review, comment
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            UNIQUE(review_id, file_name, file_type) -- Variations/versions are added in the versioned_review_files
        );
    )sql";

    constexpr const char* g_createVersionedReviewFilesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS versioned_review_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_file_id INTEGER NOT NULL,    -- foreign key to review_files.id
            version INTEGER NOT NULL,           -- version number (1, 2, 3, ...)
            file_path TEXT NOT NULL,            -- relative path to the actual file on disk
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_file_id) REFERENCES review_files(id) ON DELETE CASCADE,
            UNIQUE(review_file_id, file_path, version)     -- ensure no duplicate version numbers
        );
    )sql";

    constexpr const char* g_createCommentsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS comments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            file_id INTEGER,
            comment TEXT NOT NULL,
            audio_time_start REAL,  -- optional: timestamp of the start selection of the comment
            audio_time_end REAL,    -- optional: timestamp of the end selection of the comment     
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,
            FOREIGN KEY(file_id) REFERENCES review_files(id) ON DELETE CASCADE
        );
    )sql";

    constexpr const char* g_createVotesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS votes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            vote INT NOT NULL DEFAULT 0,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,
            UNIQUE(review_id, user_id)     -- ensures one vote per user per review
        );
    )sql";

    constexpr const char* g_createActivityTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS activity (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER,
            project_id INTEGER,
            user_id INTEGER,
            activity_type INTEGER,
            activity_text TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id) ON DELETE CASCADE,
            FOREIGN KEY(project_id) REFERENCES projects(id) ON DELETE CASCADE,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )sql";

    constexpr std::string_view g_createSessionsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at INTEGER NOT NULL,
            FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
        );
    )sql";

    constexpr int32_t g_base64EncodeVariant         = sodium_base64_VARIANT_URLSAFE_NO_PADDING;
    constexpr std::size_t g_hashOpsLimit            = crypto_pwhash_OPSLIMIT_MODERATE;
    constexpr std::size_t g_hashMemLimit            = crypto_pwhash_MEMLIMIT_MODERATE;
    constexpr int32_t g_hashAlgorithm               = crypto_pwhash_ALG_ARGON2ID13;

    constexpr std::size_t g_passwordHashSize        = crypto_box_SEEDBYTES;
    constexpr std::size_t g_saltSize                = crypto_box_SEEDBYTES;
    constexpr std::size_t g_sessionTokenSize        = 32U;
    constexpr std::size_t g_base64SessionTokenSize  = sodium_base64_ENCODED_LEN(g_sessionTokenSize, g_base64EncodeVariant);

    constexpr std::string_view g_reviewsPathName = "Reviews";

    //#define REVIEW_TEST_DATABASE_BLOCKS

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    constexpr std::chrono::milliseconds g_blockDelay = std::chrono::seconds(2);
#endif
}

review_database::review_database(const std::filesystem::path& databasePath)
    : m_database(databasePath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE, 200)
{
    SQLite::Statement enableForeignKeysStatement(m_database, "PRAGMA foreign_keys = ON;");
    enableForeignKeysStatement.exec();

    m_database.exec(g_createWorkspaceTableStatement);
    m_database.exec(g_createProjectsTableStatement);
    m_database.exec(g_createReviewsTableStatement);
    m_database.exec(g_createUsersTableStatement);
    m_database.exec(g_createReviewAuthorsTableStatement);
    m_database.exec(g_createReviewersTableStatement);
    m_database.exec(g_createReviewFilesTableStatement);
    m_database.exec(g_createVersionedReviewFilesTableStatement);
    m_database.exec(g_createCommentsTableStatement);
    m_database.exec(g_createVotesTableStatement);
    m_database.exec(g_createActivityTableStatement);
    m_database.exec(g_createSessionsTableStatement.data());
}

auto review_database::create_workspace(const std::string name) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    SQLite::Statement insertWorkspaceName(m_database, "INSERT INTO workspaces (name) VALUES (?);");
    insertWorkspaceName.bind(1, name);
    insertWorkspaceName.exec();
}

auto review_database::open_workspace(const std::string name) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    SQLite::Statement insertWorkspaceName(m_database, "INSERT OR IGNORE INTO workspaces (name) VALUES (?);");
    insertWorkspaceName.bind(1, name);
    insertWorkspaceName.exec();
}

auto review_database::get_workspace_name() const -> concurrencpp::result<std::string>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (m_database.tableExists("workspaces"))
    {
        SQLite::Statement query(m_database, "SELECT name FROM workspaces LIMIT 1;");

        while (query.executeStep())
        {
            std::string name = query.getColumn(0);
            co_return name;
        }
    }

    co_return std::string{};
}

auto review_database::create_project(const std::string name, const std::string description) -> concurrencpp::result<project_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());
    
#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    project_data newProjectData;

    if (m_database.tableExists("projects"))
    {
        SQLite::Statement query(m_database, "INSERT INTO projects (name, description) VALUES (?, ?);");
        query.bind(1, name);
        query.bind(2, description);
        query.exec();

        newProjectData.m_id = m_database.getLastInsertRowid();
        newProjectData.m_projectName = name;
        newProjectData.m_projectDescription = description;

        if (m_database.tableExists("activity"))
        {
            SQLite::Statement addActivity(m_database, "INSERT INTO activity (project_id, activity_type, activity_text) VALUES (?, ?, ?)");
            addActivity.bind(1, newProjectData.m_id);
            addActivity.bind(2, (int)activity_type::project_created);
            addActivity.bind(3, fmt::format("Created a project called {}", name));
            addActivity.exec();
        }
    }
    co_return newProjectData;
}

auto review_database::get_all_projects() const -> concurrencpp::result<std::vector<project_data>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::vector<project_data> result;

    if (m_database.tableExists("projects"))
    {
        SQLite::Statement query(m_database, "SELECT id, name, description FROM projects;");

        while (query.executeStep())
        {
            project_data projectData;
            projectData.m_id                    = query.getColumn(0).getInt();
            projectData.m_projectName           = query.getColumn(1).getString();
            projectData.m_projectDescription    = query.getColumn(2).getString();

            result.push_back(std::move(projectData));
        }
    }

    co_return result;
}

auto review_database::create_review(int64_t projectId, const new_transit_review_data newReview) -> concurrencpp::result<review_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    review_data result;

    if (!newReview.m_reviewName.empty() && projectId != 0)
    {
        if (m_database.tableExists("reviews"))
        {
            SQLite::Statement insertReviewStatement(m_database, "INSERT INTO reviews (project_id, name, task_url, description, status, phase, quality) VALUES (?, ?, ?, ?, ?, ?, ?);");
            insertReviewStatement.bind(1, projectId);
            insertReviewStatement.bind(2, newReview.m_reviewName);
            insertReviewStatement.bind(3, newReview.m_reviewTaskUrl);
            insertReviewStatement.bind(4, newReview.m_reviewDescription);
            insertReviewStatement.bind(5, (int)review_status::open);
            insertReviewStatement.bind(6, (int)newReview.m_reviewPhase);
            insertReviewStatement.bind(6, (int)newReview.m_reviewQuality);

            insertReviewStatement.exec();

            result.m_reviewId = m_database.getLastInsertRowid();
            result.m_reviewName = newReview.m_reviewName;
            result.m_reviewTaskUrl = newReview.m_reviewTaskUrl;
            result.m_reviewDescription = newReview.m_reviewDescription;
            result.m_reviewStatus      = review_status::open;
            result.m_reviewPhase       = newReview.m_reviewPhase;
            result.m_reviewQuality     = newReview.m_reviewQuality;

            if (m_database.tableExists("activity"))
            {
                SQLite::Statement addActivity(m_database, "INSERT INTO activity (review_id, activity_type, activity_text) VALUES (?, ?, ?)");
                addActivity.bind(1, result.m_reviewId);
                addActivity.bind(2, (int)activity_type::review_created);
                addActivity.bind(3, fmt::format("Created a review called {}", newReview.m_reviewName));
                addActivity.exec();
            }

            // Do file stuff on a background thread as we don't need to sync the database right now
            co_await concurrencpp::resume_on(gluten::app::get()->background_executor());

            std::unordered_set<std::string> uniqueContextFilesNames;
            std::unordered_set<std::string> uniqueReviewFilesNames;

            const std::filesystem::path databasePath = std::filesystem::path(m_database.getFilename()).parent_path();
            const std::filesystem::path reviewsPath = databasePath / g_reviewsPathName;
            const std::filesystem::path thisReviewPath = reviewsPath / std::to_string(result.m_reviewId);

            for (const auto& rawContextFile : newReview.m_contextFiles)
            {
                if (uniqueContextFilesNames.contains(rawContextFile.m_fileName))
                {
                    continue;
                }

                uniqueContextFilesNames.insert(rawContextFile.m_fileName);

                const std::filesystem::path thisFilePath = thisReviewPath / std::filesystem::path(rawContextFile.m_fileName).stem();
                const std::filesystem::path currentVersionFilePath = thisFilePath / "v1";
                const std::filesystem::path currentVersionFile     = currentVersionFilePath / rawContextFile.m_fileName;

                std::filesystem::create_directories(currentVersionFilePath);

                std::ofstream stream(currentVersionFile.string(), std::ios::binary | std::ios::out);
                stream.write(reinterpret_cast<const char*>(rawContextFile.m_fileData.data()), rawContextFile.m_fileData.size());

                versionable_review_asset versionedReviewAsset;
                versionedReviewAsset.m_fileName            = rawContextFile.m_fileName;
                versionedReviewAsset.m_versionsToRelativeFiles[1] = std::filesystem::relative(currentVersionFile, databasePath);
                result.m_relativeContextFiles.push_back(versionedReviewAsset);
            }

            for (const auto& rawReviewFile : newReview.m_reviewFiles)
            {
                if (uniqueReviewFilesNames.contains(rawReviewFile.m_fileName))
                {
                    continue;
                }

                uniqueReviewFilesNames.insert(rawReviewFile.m_fileName);

                const std::filesystem::path thisFilePath = thisReviewPath / std::filesystem::path(rawReviewFile.m_fileName).stem();
                const std::filesystem::path currentVersionFilePath = thisFilePath / "v1";
                const std::filesystem::path currentVersionFile     = currentVersionFilePath / rawReviewFile.m_fileName;

                std::filesystem::create_directories(currentVersionFilePath);

                std::ofstream stream(currentVersionFile.string(), std::ios::binary | std::ios::out);
                stream.write(reinterpret_cast<const char*>(rawReviewFile.m_fileData.data()), rawReviewFile.m_fileData.size());

                versionable_review_asset versionedReviewAsset;
                versionedReviewAsset.m_fileName            = rawReviewFile.m_fileName;
                versionedReviewAsset.m_versionsToRelativeFiles[1] = std::filesystem::relative(currentVersionFile, databasePath);
                result.m_reviewAssets.push_back(versionedReviewAsset);
            }

            uniqueContextFilesNames.clear();
            uniqueReviewFilesNames.clear();

            // Back to database thread to insert all the files
            co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

            for (auto& contextFile : result.m_relativeContextFiles)
            {
                if (uniqueContextFilesNames.contains(contextFile.m_fileName))
                {
                    continue;
                }

                uniqueContextFilesNames.insert(contextFile.m_fileName);

                SQLite::Statement insertReviewFileStatement(m_database, "INSERT INTO review_files (review_id, file_name, file_type) VALUES (?, ?, ?);");
                insertReviewFileStatement.bind(1, result.m_reviewId);
                insertReviewFileStatement.bind(2, contextFile.m_fileName);
                insertReviewFileStatement.bind(3, (int)review_file_type::context);
                insertReviewFileStatement.exec();

                contextFile.m_fileId = m_database.getLastInsertRowid();
                
                SQLite::Statement insertVersionedReviewFileStatement(m_database, "INSERT INTO versioned_review_files (review_file_id, version, file_path) VALUES (?, ?, ?);");
                insertVersionedReviewFileStatement.bind(1, m_database.getLastInsertRowid());
                insertVersionedReviewFileStatement.bind(2, 1);
                insertVersionedReviewFileStatement.bind(3, contextFile.m_versionsToRelativeFiles.at(1).string());
                insertVersionedReviewFileStatement.exec();
            }

            for (auto& reviewFile : result.m_reviewAssets)
            {
                if (uniqueReviewFilesNames.contains(reviewFile.m_fileName))
                {
                    continue;
                }

                uniqueReviewFilesNames.insert(reviewFile.m_fileName);

                SQLite::Statement insertReviewFileStatement(m_database, "INSERT INTO review_files (review_id, file_name, file_type) VALUES (?, ?, ?);");
                insertReviewFileStatement.bind(1, result.m_reviewId);
                insertReviewFileStatement.bind(2, reviewFile.m_fileName);
                insertReviewFileStatement.bind(3, (int)review_file_type::review);
                insertReviewFileStatement.exec();

                reviewFile.m_fileId = m_database.getLastInsertRowid();

                SQLite::Statement insertVersionedReviewFileStatement(m_database, "INSERT INTO versioned_review_files (review_file_id, version, file_path) VALUES (?, ?, ?);");
                insertVersionedReviewFileStatement.bind(1, m_database.getLastInsertRowid());
                insertVersionedReviewFileStatement.bind(2, 1);
                insertVersionedReviewFileStatement.bind(3, reviewFile.m_versionsToRelativeFiles.at(1).string());
                insertVersionedReviewFileStatement.exec();
            }
        }
    }

    co_return result;
}

auto review_database::update_review(const review_data review) -> concurrencpp::result<review_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (!review.m_reviewName.empty() && review.m_reviewId != 0)
    {
        if (m_database.tableExists("reviews"))
        {
            SQLite::Statement updateReviewStatement(m_database, "UPDATE reviews SET name = ?, task_url = ?, description = ?, status = ?, phase = ?, quality = ? WHERE id = ?;");
            updateReviewStatement.bind(1, review.m_reviewName);
            updateReviewStatement.bind(2, review.m_reviewTaskUrl);
            updateReviewStatement.bind(3, review.m_reviewDescription);
            updateReviewStatement.bind(4, (int)review.m_reviewStatus);
            updateReviewStatement.bind(5, (int)review.m_reviewPhase);
            updateReviewStatement.bind(6, (int)review.m_reviewQuality);
            updateReviewStatement.bind(7, review.m_reviewId);

            updateReviewStatement.exec();

            if (m_database.tableExists("activity"))
            {
                SQLite::Statement editActivity(m_database, "INSERT INTO activity (review_id, activity_type, activity_text) VALUES (?, ?, ?)");
                editActivity.bind(1, review.m_reviewId);
                editActivity.bind(2, (int)activity_type::review_edited);
                editActivity.bind(3, fmt::format("Edited a review called {}", review.m_reviewName));
                editActivity.exec();
            }
        }
    }

    co_return review;
}

auto review_database::get_all_reviews(int64_t projectId) const -> concurrencpp::result<std::vector<review_data>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::vector<review_data> result;

    if (m_database.tableExists("reviews") && projectId != 0)
    {
        SQLite::Statement query(m_database, "SELECT id, name, description, task_url, status, phase, quality FROM reviews WHERE project_id=?;");
        query.bind(1, projectId);

        while (query.executeStep())
        {
            review_data reviewData;
            reviewData.m_reviewId           = query.getColumn(0).getInt();
            reviewData.m_reviewName         = query.getColumn(1).getString();
            reviewData.m_reviewDescription  = query.getColumn(2).getString();
            reviewData.m_reviewTaskUrl      = query.getColumn(3).getString();
            reviewData.m_reviewStatus       = (review_status)query.getColumn(4).getInt();
            reviewData.m_reviewPhase        = (review_phase)query.getColumn(5).getInt();
            reviewData.m_reviewQuality      = (review_quality)query.getColumn(6).getInt();

            std::unordered_map<int64_t, versionable_review_asset> contextFilesMap;
            std::unordered_map<int64_t, versionable_review_asset> reviewFilesMap;
            std::unordered_map<int64_t, versionable_review_asset> commentFilesMap;

            SQLite::Statement getFilesStatement(m_database, "SELECT id, file_name, file_type FROM review_files WHERE review_id = ?;");
            getFilesStatement.bind(1, reviewData.m_reviewId);

            while (getFilesStatement.executeStep())
            {
                const int64_t fileId            = getFilesStatement.getColumn(0).getInt64();
                const std::string fileName      = getFilesStatement.getColumn(1).getText();
                const review_file_type fileType = (review_file_type)getFilesStatement.getColumn(2).getInt();

                SQLite::Statement getVersionedFilesStatement(m_database, "SELECT version, file_path FROM versioned_review_files WHERE review_file_id = ?;");
                getVersionedFilesStatement.bind(1, fileId);

                if (getVersionedFilesStatement.executeStep())
                {
                    const std::size_t fileVersion = getVersionedFilesStatement.getColumn(0).getInt64();
                    const std::filesystem::path relativeFilePath = getVersionedFilesStatement.getColumn(1).getText();

                    switch ((review_file_type)getFilesStatement.getColumn(2).getInt())
                    {
                        case review_file_type::context:
                            contextFilesMap[fileId].m_fileId   = fileId;
                            contextFilesMap[fileId].m_fileName = fileName;
                            contextFilesMap[fileId].m_versionsToRelativeFiles.insert({fileVersion, relativeFilePath});
                            break;
                        case review_file_type::review:
                            reviewFilesMap[fileId].m_fileId   = fileId;
                            reviewFilesMap[fileId].m_fileName = fileName;
                            reviewFilesMap[fileId].m_versionsToRelativeFiles.insert({fileVersion, relativeFilePath});
                            break;
                        case review_file_type::comment:
                            commentFilesMap[fileId].m_fileId   = fileId;
                            commentFilesMap[fileId].m_fileName = fileName;
                            commentFilesMap[fileId].m_versionsToRelativeFiles.insert({fileVersion, relativeFilePath});
                            break;
                    }
                }
            }

            if (!contextFilesMap.empty())
            {
                reviewData.m_relativeContextFiles.resize(contextFilesMap.size());
                std::transform(contextFilesMap.begin(), contextFilesMap.end(), reviewData.m_relativeContextFiles.begin(), [](const auto& pair) { return pair.second; });
            }

            if (!reviewFilesMap.empty())
            {
                reviewData.m_reviewAssets.resize(reviewFilesMap.size());
                std::transform(reviewFilesMap.begin(), reviewFilesMap.end(), reviewData.m_reviewAssets.begin(), [](const auto& pair) { return pair.second; });
            }

            result.push_back(std::move(reviewData));
        }
    }

    co_return result;
}

auto review_database::get_user_vote_on_review(int64_t reviewId, int64_t userId) const -> concurrencpp::result<tl::expected<review_vote, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (userId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "User ID is invalid"});
    }

    if (reviewId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Review ID is invalid"});
    }

    if (!m_database.tableExists("votes"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "Votes table is empty"});
    }

    review_vote result = review_vote::no_vote;

    SQLite::Statement getVoteStatement(m_database, "SELECT vote FROM votes WHERE review_id = ? AND user_id = ?;");
    getVoteStatement.bind(1, reviewId);
    getVoteStatement.bind(2, userId);

    while (getVoteStatement.executeStep())
    {
        result = (review_vote)getVoteStatement.getColumn(0).getInt();
    }

    co_return result;
}

auto review_database::delete_review(int64_t reviewId, std::string userToken) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (reviewId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Review ID is invalid"});
    }

    if (userToken.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "Session token is invalid"});
    }

    if (!co_await user_is_logged_in_and_has_privilege_for_action(userToken, activity_type::review_deleted))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "User is unauthorized to delete a review"});
    }

    // TOOD: Reviews need to be authored by someone so we can delete reviews only if the user created it (or is an admin)

    if (!m_database.tableExists("reviews"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No reviews table"});
    }

    if (!m_database.tableExists("activity"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No activity table"});
    }

    if (!m_database.tableExists("comments"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No comments table"});
    }

    if (!m_database.tableExists("review_files"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No review_files table"});
    }

    if (!m_database.tableExists("versioned_review_files"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No versioned_review_files table"});
    }
        
    // Cascading delete removes all other data referencing this
    SQLite::Statement deleteReviewStatement(m_database, "DELETE FROM reviews WHERE id = ?;");
    deleteReviewStatement.bind(1, reviewId);
    deleteReviewStatement.exec();

    co_return true;
}

auto review_database::get_all_activity_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<activity_data>> 
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::vector<activity_data> result;

    if (m_database.tableExists("activity") && reviewId != 0)
    {
        SQLite::Statement query(m_database, "SELECT id, review_id, project_id, user_id, activity_type, activity_text, timestamp FROM activity WHERE review_id=?;");
        query.bind(1, reviewId);

        while (query.executeStep())
        {
            activity_data activityData;
            activityData.m_activityId        = query.getColumn(0).getInt64();
            activityData.m_reviewId          = query.getColumn(1).getInt64();
            activityData.m_projectId         = query.getColumn(2).getInt64();
            activityData.m_userId            = query.getColumn(3).getInt64();
            activityData.m_activityType      = (activity_type)query.getColumn(4).getInt();
            activityData.m_activityText      = query.getColumn(5).getString();
            activityData.m_activityTimestamp = query.getColumn(6).getString();

            result.push_back(std::move(activityData));
        }
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {return lhs.m_activityId > rhs.m_activityId;});

    co_return result;
}

auto review_database::get_all_comments_for_review(int64_t reviewId) const -> concurrencpp::result<std::vector<comment_data>> 
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::vector<comment_data> result;

    if (m_database.tableExists("comments") && reviewId != 0)
    {
        SQLite::Statement query(m_database, "SELECT id, review_id, user_id, comment, timestamp, audio_time_start, audio_time_end, file_id FROM comments WHERE review_id=?;");
        query.bind(1, reviewId);

        while (query.executeStep())
        {
            comment_data commentData;
            commentData.m_commentId         = query.getColumn(0).getInt64();
            commentData.m_reviewId          = query.getColumn(1).getInt64();
            commentData.m_userId            = query.getColumn(2).getInt64();
            commentData.m_comment           = query.getColumn(3).getText();
            commentData.m_timestamp         = query.getColumn(4).getString();
            commentData.m_timeStart         = query.getColumn(5).getDouble();
            commentData.m_timeEnd           = query.getColumn(6).getDouble();
            commentData.m_fileId            = query.getColumn(7).getInt64();

            result.push_back(std::move(commentData));
        }
    }

    std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) { return lhs.m_commentId > rhs.m_commentId; });

    co_return result;
}

auto review_database::create_comment(new_comment_data newComment) -> concurrencpp::result<comment_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    constexpr float defaultAudioStartOrEnd = -1.0f;

    if (!newComment.m_comment.empty() && newComment.m_reviewId != 0)
    {
        if (m_database.tableExists("comments"))
        {
            SQLite::Statement insertCommentStatement(m_database,"INSERT INTO comments (review_id, user_id, comment, audio_time_start, audio_time_end, file_id) VALUES (?, ?, ?, ?, ?, ?);");

            insertCommentStatement.bind(1, newComment.m_reviewId);
            insertCommentStatement.bind(2, newComment.m_userId);
            insertCommentStatement.bind(3, newComment.m_comment);
            insertCommentStatement.bind(4, newComment.m_timeStart);
            insertCommentStatement.bind(5, newComment.m_timeEnd);
            insertCommentStatement.bind(6, newComment.m_fileId);

            insertCommentStatement.exec();
            
            if (m_database.tableExists("activity"))
            {
                SQLite::Statement addActivity(m_database, "INSERT INTO activity (review_id, activity_type, activity_text) VALUES (?, ?, ?);");
                addActivity.bind(1, newComment.m_reviewId);
                addActivity.bind(2, (int)activity_type::comment_added);
                addActivity.bind(3, fmt::format("Created a comment -> \"{}\"", newComment.m_comment));
                addActivity.exec();
            }
        }
    }

    co_return comment_data{};
}

auto review_database::delete_comment(int64_t commentId) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (commentId != 0)
    {
        if (m_database.tableExists("comments"))
        {
            if (m_database.tableExists("activity"))
            {
                SQLite::Statement addActivity(m_database, "INSERT INTO activity (review_id, activity_type, activity_text) SELECT review_id, ?, ? FROM comments WHERE id=? LIMIT 1;");
                addActivity.bind(1, (int)activity_type::comment_deleted);
                addActivity.bind(2, fmt::format("Removed a comment"));
                addActivity.bind(3, commentId);
                addActivity.exec();
            }

            SQLite::Statement insertCommentStatement(m_database, "DELETE FROM comments where id=?");
            insertCommentStatement.bind(1, commentId);
            insertCommentStatement.exec();
        }
    }

    co_return;
}

auto review_database::create_user(new_user_data newUser, std::string userToken) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (newUser.m_email.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Email address is empty"});
    }

    if (newUser.m_rawPassword.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Password is empty"});
    }

    const bool firstUserCreation = userToken.empty() && user_table_is_empty();
    const bool userLoggedIn      = !firstUserCreation && user_is_logged_in_and_has_privilege_for_action(userToken, activity_type::user_added);

    if (!firstUserCreation && !userLoggedIn)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "User is unauthorized to create a user"});
    }

    if (!m_database.tableExists("users"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "Users table does not exist"});
    }

    unsigned char hashBuffer[g_passwordHashSize] = {0};
    unsigned char saltBuffer[g_saltSize]         = {0};

    randombytes_buf(saltBuffer, g_saltSize);

    if (crypto_pwhash(
        hashBuffer,
        g_passwordHashSize,
        newUser.m_rawPassword.data(),
        g_rawPasswordSize,
        saltBuffer,
        g_hashOpsLimit,
        g_hashMemLimit,
        g_hashAlgorithm) != 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::error, .m_errorMessage = "Could not hash the password"});
    }
     
    SQLite::Statement insertUserStatement(m_database, "INSERT INTO users (display_name, title, email, password, salt, privilege) VALUES (?, ?, ?, ?, ?, ?);");
    insertUserStatement.bind(1, newUser.m_displayName);
    insertUserStatement.bind(2, newUser.m_title);
    insertUserStatement.bind(3, newUser.m_email);
    insertUserStatement.bind(4, reinterpret_cast<char*>(hashBuffer));
    insertUserStatement.bind(5, reinterpret_cast<char*>(saltBuffer));
    insertUserStatement.bind(6, (int)newUser.m_requestedPrivileges);
    insertUserStatement.exec();

    if (m_database.tableExists("activity"))
    {
        SQLite::Statement addActivity(m_database, "INSERT INTO activity (activity_type, activity_text) VALUES (?, ?);");
        addActivity.bind(1, (int)activity_type::user_added);
        addActivity.bind(2, fmt::format("Added {} as a user", newUser.m_email));
        addActivity.exec();
    }

    co_return true;
}

auto review_database::user_table_is_empty() const -> concurrencpp::result<bool>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::size_t usersCount = 0;

    if (m_database.tableExists("users"))
    {
        SQLite::Statement getsUsersCountStatement(m_database, "SELECT COUNT(id) FROM users;");
        if (getsUsersCountStatement.executeStep())
        {
            usersCount = getsUsersCountStatement.getColumn(0).getInt64();
        }
    }

    co_return usersCount == 0;
}

auto review_database::user_is_logged_in_and_has_privilege_for_action(std::string userToken, activity_type activity) -> concurrencpp::result<bool>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    user_privileges requiredPriveleges = user_privileges::admin;

    switch (activity)
    {
    case activity_type::project_created:
    case activity_type::project_edited:
    case activity_type::project_deleted:
    case activity_type::user_added:
    case activity_type::user_edited:
    case activity_type::user_deleted:
        requiredPriveleges = user_privileges::admin;
        break;
    case activity_type::review_created:
    case activity_type::review_edited:
    case activity_type::review_file_deleted:
    case activity_type::review_files_edited:
    case activity_type::comment_added:
    case activity_type::comment_edited:
    case activity_type::comment_deleted:
        requiredPriveleges = user_privileges::user; // Users can do all these actions but will need additional checks like ensuring they only delete their own review
        break;
    default:
        break;
    }

    co_return co_await user_is_logged_in_and_has_privilege(userToken, requiredPriveleges);
}

auto review_database::user_is_logged_in_and_has_privilege(std::string userToken, user_privileges privilege) -> concurrencpp::result<bool>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    bool userWithPrivelegesIsLoggedIn = false;

    if (m_database.tableExists("sessions"))
    {
        SQLite::Statement statement(m_database,
            "SELECT users.id, users.privilege FROM sessions JOIN users ON users.id = sessions.user_id WHERE "
            "sessions.token = ? AND sessions.expires_at > strftime('%s', 'now') AND users.privilege >= ?;");
        statement.bind(1, userToken);
        statement.bind(2, (int)privilege);
        userWithPrivelegesIsLoggedIn = statement.executeStep();
    }

    co_return userWithPrivelegesIsLoggedIn;
}

auto review_database::login_user(login_request_data loginRequest) -> concurrencpp::result<tl::expected<logged_in_user_data, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (!m_database.tableExists("users"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "Missing users table"});
    }

    SQLite::Statement statement(m_database, "SELECT id, password, salt, privilege, display_name, title FROM users WHERE email = ?;");
    statement.bind(1, loginRequest.m_email);

    if (!statement.executeStep())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::no_data, .m_errorMessage = "User does not exist"});
    }

    const std::string passwordHash = (const char*)statement.getColumn(1).getBlob();
    const std::string passwordSalt = statement.getColumn(2).getString();

    unsigned char hashBuffer[g_passwordHashSize] = { 0 };

    if (crypto_pwhash(
        hashBuffer,
        g_passwordHashSize,
        loginRequest.m_rawPassword.data(),
        g_rawPasswordSize,
        reinterpret_cast<const unsigned char*>(passwordSalt.c_str()),
        g_hashOpsLimit,
        g_hashMemLimit,
        g_hashAlgorithm) != 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::error, .m_errorMessage = "Could not hash the password"});
    }

    if (sodium_memcmp(passwordHash.c_str(), hashBuffer, g_passwordHashSize) != 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "Password is incorrect"});
    }

    const int64_t userId           = statement.getColumn(0).getInt64();
    const user_privileges privilege = (user_privileges)statement.getColumn(3).getInt();
    const std::string displayName   = statement.getColumn(4).getString();
    const std::string title         = statement.getColumn(5).getString();
    
    SQLite::Statement sessionStatement(m_database, "SELECT token FROM sessions WHERE user_id = ? AND expires_at > strftime('%s', 'now')");
    sessionStatement.bind(1, userId);
    const bool hasSessionToken = sessionStatement.executeStep();

    logged_in_user_data result;
    result.m_userId       = userId;
    result.m_privileges   = privilege;
    result.m_email        = loginRequest.m_email;
    result.m_displayName  = displayName;
    result.m_title        = title;

    if (hasSessionToken)
    {
        result.m_sessionToken = sessionStatement.getColumn(0).getText();
    }
    else
    {
        unsigned char sessionToken[g_sessionTokenSize] = {0};
        char base64SessionToken[g_base64SessionTokenSize] = {0};

        randombytes_buf(sessionToken, g_sessionTokenSize);
        sodium_bin2base64(base64SessionToken, g_base64SessionTokenSize, sessionToken, g_sessionTokenSize, g_base64EncodeVariant);

        const std::time_t now    = std::time(nullptr);
        const std::time_t expiry = now + 7 * 24 * 60 * 60;

        SQLite::Statement newSessionStatement(m_database, "INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, ?)");
        newSessionStatement.bind(1, base64SessionToken);
        newSessionStatement.bind(2, userId);
        newSessionStatement.bind(3, static_cast<int>(expiry));
        newSessionStatement.exec();

        result.m_sessionToken = base64SessionToken;
    }

    co_return result;
}

auto review_database::get_all_users(std::string userToken) -> concurrencpp::result<tl::expected<std::vector<user_data>, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());
    
#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    std::vector<user_data> result;

    if (co_await user_is_logged_in_and_has_privilege(userToken, user_privileges::user))
    {
        if (m_database.tableExists("users"))
        {
            SQLite::Statement statement(m_database, "SELECT id, display_name, title, email, timestamp, privilege FROM users");

            while (statement.executeStep())
            {
                user_data user;
                user.m_userId      = statement.getColumn(0).getInt64();
                user.m_displayName = statement.getColumn(1).getText();
                user.m_title       = statement.getColumn(2).getText();
                user.m_email       = statement.getColumn(3).getText();
                user.m_createdAt   = statement.getColumn(4).getText();
                user.m_privileges  = (user_privileges)statement.getColumn(5).getInt();
                result.push_back(std::move(user));
            }
        }
        else
        {
            co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No users table"});
        }
    }
    else
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "Not authorized to view users"});
    }

    co_return result;
}

auto review_database::delete_user(std::string email, std::string userToken) -> concurrencpp::result<tl::expected<bool, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (email.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Email address was empty"});
    }

    if (userToken.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "Session token was invalid"});
    }

    if (!m_database.tableExists("users"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No users table"});
    }

    if (!m_database.tableExists("sessions"))
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::missing_table, .m_errorMessage = "No sessions table"});
    }

    SQLite::Statement deletingSelfStatement(m_database, "SELECT users.id, users.email FROM sessions JOIN users ON users.id = sessions.user_id WHERE sessions.token = ? AND users.email = ?;");
    deletingSelfStatement.bind(1, userToken);
    deletingSelfStatement.bind(2, email);

    const bool deletingSelf = deletingSelfStatement.executeStep();

    if (deletingSelf || co_await user_is_logged_in_and_has_privilege(userToken, user_privileges::admin))
    {
        SQLite::Statement deleteUserStatement(m_database, "DELETE FROM users WHERE email = ?;");
        deleteUserStatement.bind(1, email);
        deleteUserStatement.exec();
        co_return true;
    }

    co_return false;
}

auto review_database::get_users_for_review(int64_t reviewId, std::string userToken) -> concurrencpp::result<tl::expected<std::vector<user_data>, database_error>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (reviewId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Review Id is invalid"});
    }

    if (userToken.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "User token is empty"});
    }

    if (co_await user_is_logged_in_and_has_privilege(userToken, user_privileges::guest) == false)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "Unauthorized"});
    }

    SQLite::Statement getUsersForReviewStatement(m_database, "SELECT user_id FROM reviewers WHERE review_id = ?;");
    getUsersForReviewStatement.bind(1, reviewId);

    std::vector<int64_t> userIds;

    while (getUsersForReviewStatement.executeStep())
    {
        userIds.push_back(getUsersForReviewStatement.getColumn(0).getInt64());
    }

    std::vector<user_data> users;

    for (const auto& userId : userIds)
    {
        SQLite::Statement getUserStatement(m_database, "SELECT id, display_name, title, email, timestamp, privilege FROM users WHERE id = ?;");
        getUserStatement.bind(1, userId);

        if (getUserStatement.executeStep())
        {
            user_data user;
            user.m_userId      = getUserStatement.getColumn(0).getInt64();
            user.m_displayName = getUserStatement.getColumn(1).getText();
            user.m_title       = getUserStatement.getColumn(2).getText();
            user.m_email       = getUserStatement.getColumn(3).getText();
            user.m_createdAt   = getUserStatement.getColumn(4).getText();
            user.m_privileges  = (user_privileges)getUserStatement.getColumn(5).getInt();
            users.push_back(std::move(user));
        }
    }

    co_return users;
}

auto review_database::set_users_for_review(int64_t reviewId, std::vector<int64_t> userIds, std::string userToken) -> bool_result
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (reviewId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Review Id is invalid"});
    }

    if (userToken.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "User token is empty"});
    }

    SQLite::Statement removeAllReviewersStatement(m_database, "DELETE FROM reviewers WHERE review_id = ?;");
    removeAllReviewersStatement.bind(1, reviewId);
    removeAllReviewersStatement.exec();

    for (const auto& userId : userIds)
    {
        SQLite::Statement addUserStatement(m_database, "INSERT INTO reviewers (review_id, user_id) VALUES (?, ?);");
        addUserStatement.bind(1, reviewId);
        addUserStatement.bind(2, userId);
        addUserStatement.exec();
    }

    co_return true;
}

auto review_database::set_user_vote_for_review(int64_t reviewId, user_id userId, review_vote vote, std::string userToken) -> bool_result
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

#ifdef REVIEW_TEST_DATABASE_BLOCKS
    co_await review_app::get()->timer_queue()->make_delay_object(g_blockDelay, review_app::get()->get_database_thread_executor());
#endif

    if (reviewId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "Review ID is invalid"});
    }

    if (userId < 0)
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::invalid_parameters, .m_errorMessage = "User ID is invalid"});
    }

    if (userToken.empty())
    {
        co_return tl::make_unexpected(database_error{.m_errorCode = database_error_code::unauthorized, .m_errorMessage = "User token is empty"});
    }

    SQLite::Statement setVoteStatement(m_database, "INSERT OR REPLACE INTO votes (review_id, user_id, vote) VALUES (?, ?, ?);");
    setVoteStatement.bind(1, reviewId);
    setVoteStatement.bind(2, userId);
    setVoteStatement.bind(3, (int)vote);
    setVoteStatement.exec();

    co_return true;
}