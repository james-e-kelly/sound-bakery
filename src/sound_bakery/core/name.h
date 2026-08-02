#include "sound_bakery/pch.h"

namespace sbk::core
{
    /**
     * @brief Specialised string that can check for illegal characters.
     */
    class SB_CLASS name
    {
    public:
        name() = delete;
        name(std::string_view data, std::string_view illegalCharacters)
            : m_data(data),
              m_illegalCharacters(illegalCharacters)
        {
            BOOST_ASSERT_MSG(!m_data.empty(), "Strings cannot be set to empty");
        }

        auto test_set(std::string_view data) -> bool
        {
            return !data.empty() && data.find_first_of(m_illegalCharacters) == std::string::npos;
        }

        auto set(std::string_view data, bool skipTest = false) -> bool
        {
            if (skipTest || test_set(data))
            {
                m_data = data;
                return true;
            }
            return false;
        }

        auto get() const -> std::string_view
        {
            return m_data;
        }

        operator std::string_view() const
        {
            return get();
        }

        auto operator=(std::string_view data) -> bool
        {
            return set(data);
        }

    private:
        std::string m_data;
        std::string m_illegalCharacters;
    };

    struct SB_CLASS parsed_database_name
    {
        parsed_database_name() = delete;
        parsed_database_name(const struct database_name* databaseName);

        std::string objectType;
        std::string objectPath;
        std::string objectName;
    };

    /**
     * @brief Unique and searchable name of type, path and name.
     *
     * Database strings look like "event:/play_my_event" or "random:/my/path/my_node_object".
     *
     * Database strings have the format "{type}:{path}/{name}".
     */
    struct SB_CLASS database_name
    {
        database_name() = default;
        explicit database_name(const std::string& name) : databaseName(name) {}
        explicit database_name(const std::string_view& type, const std::string_view& objectName)
            : databaseName(fmt::format("{}:/{}", type, objectName))
        {
        }
        explicit database_name(const parsed_database_name& parsedDatabaseName)
            : databaseName(fmt::format("{}:{}{}/{}",
                                       parsedDatabaseName.objectType,
                                       parsedDatabaseName.objectPath.empty() ? "" : "/",
                                       parsedDatabaseName.objectPath,
                                       parsedDatabaseName.objectName))
        {
        }

        [[nodiscard]] auto parse() const -> parsed_database_name;  //< Split the database name into its component parts
        [[nodiscard]] auto valid() const -> bool;

        operator std::string() const { return databaseName; }
        operator std::string_view() const { return databaseName; }
        operator const char*() const { return databaseName.c_str(); }
        auto operator==(const database_name& other) const -> bool { return databaseName.compare(other.databaseName) == 0; }

        auto operator/=(const std::string_view& data) -> database_name&
        {
            databaseName.append("/").append(data);
            return *this;
        }

        [[nodiscard]] friend auto operator/(const database_name& left, const std::string_view& data) -> database_name
        {
            database_name temp = left;
            temp /= data;
            return temp;
        }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int) -> void
        {
            archive& boost::serialization::make_nvp("Name", databaseName);
        }

        std::string databaseName;
    };
}  // namespace sbk::core