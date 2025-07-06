#include "review_database.h"

#include "app/review_app.h"

namespace
{
    constexpr const char* g_workspaceTableName = "workspace";
    constexpr const char* g_projectsTableName = "projects";
    constexpr const char* g_reviewsTableName = "reviews";
    constexpr const char* g_commentsTableName = "comments";

    constexpr const char* g_idColumnName            = "id";
    constexpr const char* g_nameColumnName          = "name";
    constexpr const char* g_descriptionColumnName   = "description";
}

review_database::review_database(const std::filesystem::path& databasePath)
    : m_database(databasePath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE, 200)
{
    m_database.exec(fmt::format("CREATE TABLE IF NOT EXISTS {} ({} INTEGER PRIMARY KEY AUTOINCREMENT, {} TEXT);", g_workspaceTableName, g_idColumnName, g_nameColumnName).c_str());
    m_database.exec(fmt::format("CREATE TABLE IF NOT EXISTS {} ({} INTEGER PRIMARY KEY AUTOINCREMENT, {} TEXT, {} TEXT);", g_projectsTableName, g_idColumnName, g_nameColumnName, g_descriptionColumnName).c_str());
}

auto review_database::create_workspace(const std::string name) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    SQLite::Statement insertWorkspaceName(m_database, fmt::format("INSERT INTO {} ({}) VALUES (?);", g_workspaceTableName, g_descriptionColumnName).c_str());
    insertWorkspaceName.bind(g_nameColumnName, name);

    insertWorkspaceName.exec();
}

auto review_database::get_workspace_name() const -> concurrencpp::result<std::string>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    if (m_database.tableExists(g_workspaceTableName))
    {
        SQLite::Statement query(m_database, fmt::format("SELECT {} FROM workspace LIMIT 1;", g_nameColumnName).c_str());

        while (query.executeStep())
        {
            std::string name = query.getColumn(0);
            co_return name;
        }
    }

    co_return std::string{};
}

auto review_database::create_project(const std::string name, const std::string description) -> concurrencpp::result<void>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    if (m_database.tableExists(g_projectsTableName))
    {
        SQLite::Statement query(m_database, fmt::format("INSERT INTO {} ({}, {}) VALUES (?, ?);", g_projectsTableName, g_nameColumnName, g_descriptionColumnName).c_str());
        query.bind(1, name);
        query.bind(2, description);

        query.exec();
    }
}

auto review_database::get_all_projects() const -> concurrencpp::result<std::vector<project_data>>
{
    co_await concurrencpp::resume_on(review_app::get()->get_database_thread_executor());

    std::vector<project_data> result;

    if (m_database.tableExists(g_projectsTableName))
    {
        SQLite::Statement query(m_database, fmt::format("SELECT {}, {}, {} FROM {};", g_idColumnName, g_nameColumnName, g_descriptionColumnName, g_projectsTableName).c_str());

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