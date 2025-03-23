#pragma once

#include "sound_bakery/core/object/object.h"

namespace sbk::core
{
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
            : databaseName(fmt::format("{}:/{}", type, objectName)) {}
        explicit database_name(const parsed_database_name& parsedDatabaseName)
            : databaseName(fmt::format("{}:{}{}/{}",
                                       parsedDatabaseName.objectType,
                                       parsedDatabaseName.objectPath.empty() ? "" : "/", parsedDatabaseName.objectPath,
                                       parsedDatabaseName.objectName))
        {
        }

        auto parse() const -> parsed_database_name; //< Split the database name into its component parts
        auto valid() const -> bool;

        operator std::string() const { return databaseName; }
        operator std::string_view() const { return databaseName; }
        operator const char*() const { return databaseName.c_str(); }
        bool operator==(const database_name& other) const
        {
            return databaseName.compare(other.databaseName) == 0;
        }

        database_name& operator/=(const std::string_view& data)
        {
            databaseName.append("/").append(data);
            return *this;
        }

        [[nodiscard]] friend database_name operator/(const database_name& left, const std::string_view& data)
        {
            database_name temp = left;
            temp /= data;
            return temp;
        }

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            archive & boost::serialization::make_nvp("Name", databaseName);
        }
        
        std::string databaseName;
    };

    struct database_name_comparator
    {
        bool operator()(const database_name& lhs, const database_name& rhs) const;
    };

    /**
     * @brief Base object type for any object that can exist in the
     * editor/database. Holds an ID and name
     */
    class SB_CLASS database_object : public object
    {
        REGISTER_REFLECTION(database_object, object)
        LEAK_DETECTOR(database_object)

    public:
        using update_id_delegate = MulticastDelegate<sbk_id, sbk_id>;
        using update_database_name_delegate = MulticastDelegate<const database_name&, const database_name&>;

        database_object();
        virtual ~database_object();

        [[nodiscard]] auto get_database_id() const -> sbk_id;                   //< Get the unique indentifier of the object
        [[nodiscard]] auto get_asset_name() const -> std::string;               //< Get the filename/asset name of the object
        [[nodiscard]] auto get_database_path(std::string& path) const -> void;  //< Get the absolute path name of the object
        [[nodiscard]] auto get_database_name() const -> database_name;          //< Get a unique name that uses the type, path, and name
        [[nodiscard]] auto get_editor_hidden() const -> bool;

        auto set_database_id(sbk_id id) -> void;
        auto set_editor_hidden(bool hidden) -> void;

        operator sbk_id() const { return m_objectID; }

        [[nodiscard]] auto get_on_update_id() -> update_id_delegate&;
        [[nodiscard]] auto get_on_update_database_name() -> update_database_name_delegate&;

    private:
        auto on_update_name(std::string_view oldName, std::string_view newName) -> void;
        DelegateHandle m_onUpdateNameHandle;

        update_id_delegate m_onUpdateID;
        update_database_name_delegate m_onUpdateDatabaseName;
        sbk_id m_objectID = 0;
        bool editorHidden = false;  //< If true, the object won't render in the editor or be saved

        friend class database;
        friend struct object_ptr_comparator;
    };
}  // namespace sbk::core

namespace std
{
    template<>
    struct hash<sbk::core::database_name>
    {
        size_t operator()(const sbk::core::database_name& k) const { return hash<std::string>{}(k.databaseName); }
    };
}  // namespace std