#include "review_database.h"

#include "app/review_app.h"

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
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )sql";

    constexpr const char* g_createUsersTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            display_name TEXT,
            title TEXT,
            email TEXT,
            password TEXT,   -- for now we will allow null passwords for easy logins
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            privilege INT NOT NULL DEFAULT 0
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
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(project_id) REFERENCES projects(id)
        );
    )sql";

    constexpr const char* g_createReviewAuthorsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS review_authors (
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            PRIMARY KEY (review_id, user_id),
            FOREIGN KEY(review_id) REFERENCES reviews(id),
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )sql";

    constexpr const char* g_createReviewFilesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS review_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            file_name TEXT NOT NULL,          -- logical name (e.g. "footstep_sfx")
            file_type TEXT,                   -- audio, video, ref, etc.
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id)
        );
    )sql";

    constexpr const char* g_createVersionedReviewFilesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS versioned_review_files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_file_id INTEGER NOT NULL,  -- foreign key to review_files.id
            version INTEGER NOT NULL,          -- version number (1, 2, 3, ...)
            file_path TEXT NOT NULL,           -- relative path to the actual file on disk
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_file_id) REFERENCES review_files(id),
            UNIQUE(review_file_id, version)   -- ensure no duplicate version numbers
        );
    )sql";

    constexpr const char* g_createCommentsTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS comments (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            message TEXT NOT NULL,
            audio_time REAL,               -- optional: timestamp in media
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id),
            FOREIGN KEY(user_id) REFERENCES users(id)
        );
    )sql";

    constexpr const char* g_createVotesTableStatement = R"sql(
        CREATE TABLE IF NOT EXISTS votes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            review_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            vote TEXT NOT NULL,            -- e.g. "approve", "reject", "needs_work"
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(review_id) REFERENCES reviews(id),
            FOREIGN KEY(user_id) REFERENCES users(id),
            UNIQUE(review_id, user_id)     -- ensures one vote per user per review
        );
    )sql";
}

review_database::review_database(const std::filesystem::path& databasePath)
    : m_database(databasePath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE, 200)
{
    m_database.exec(g_createWorkspaceTableStatement);
    m_database.exec(g_createProjectsTableStatement);
    m_database.exec(g_createReviewsTableStatement);
    m_database.exec(g_createUsersTableStatement);
    m_database.exec(g_createReviewAuthorsTableStatement);
    m_database.exec(g_createReviewFilesTableStatement);
    m_database.exec(g_createVersionedReviewFilesTableStatement);
    m_database.exec(g_createCommentsTableStatement);
    m_database.exec(g_createVotesTableStatement);
}

auto review_database::create_workspace(const std::string name) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    SQLite::Statement insertWorkspaceName(m_database, "INSERT INTO workspaces (name) VALUES (?);");
    insertWorkspaceName.bind(1, name);
    insertWorkspaceName.exec();
}

auto review_database::open_workspace(const std::string name) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    SQLite::Statement insertWorkspaceName(m_database, "INSERT OR IGNORE INTO workspaces (name) VALUES (?);");
    insertWorkspaceName.bind(1, name);
    insertWorkspaceName.exec();
}

auto review_database::get_workspace_name() const -> concurrencpp::result<std::string>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

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
    }
    co_return newProjectData;
}

auto review_database::get_all_projects() const -> concurrencpp::result<std::vector<project_data>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

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

auto review_database::create_review(int64_t projectId, const new_review_data newReview) -> concurrencpp::result<review_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

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
        }
    }

    co_return result;
}

auto review_database::update_review(const review_data review) -> concurrencpp::result<review_data>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

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
        }
    }

    co_return review;
}

auto review_database::get_all_reviews(int64_t projectId) const -> concurrencpp::result<std::vector<review_data>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

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

            result.push_back(std::move(reviewData));
        }
    }

    co_return result;
}