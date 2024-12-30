#pragma once

#include "sound_bakery/core/object/object.h"

namespace sbk::core
{
    /**
     * @brief Base object type for any object that can exist in the
     * editor/database. Holds an ID and name
     */
    class SB_CLASS database_object : public object
    {
        REGISTER_REFLECTION(database_object, object)
        LEAK_DETECTOR(database_object)

    public:
        [[nodiscard]] auto get_database_id() const -> sbk_id;
        [[nodiscard]] auto get_database_name() const -> std::string_view;
        [[nodiscard]] auto get_editor_hidden() const -> bool;

        auto set_database_id(sbk_id id) -> void;
        auto set_database_name(std::string_view name) -> void;
        auto set_editor_hidden(bool hidden) -> void;

        operator sbk_id() const { return m_objectID; }

        [[nodiscard]] auto get_on_update_id() -> MulticastDelegate<sbk_id, sbk_id>&;
        [[nodiscard]] auto get_on_update_name() -> MulticastDelegate<std::string_view, std::string_view>&;

    private:
        MulticastDelegate<sbk_id, sbk_id> m_onUpdateID;
        MulticastDelegate<std::string_view, std::string_view> m_onUpdateName;

        std::string m_objectName;
        sbk_id m_objectID = 0;

        bool editorHidden = false;  //< If true, the object won't render in the editor or be saved

        friend class database;
    };
}  // namespace sbk::core