#pragma once

#include "sound_bakery/core/object/object.h"

namespace sbk::core
{
    

    struct database_name_comparator
    {
        auto operator()(const database_name& lhs, const database_name& rhs) const -> bool;
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
        [[nodiscard]] virtual auto get_is_export() const -> bool;           //< Whether this object should be exported and made public to integrations. In Unreal, any object exported will end up as a UAsset

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
        auto operator()(const sbk::core::database_name& k) const -> size_t { return hash<std::string>{}(k.databaseName); }
    };
}  // namespace std