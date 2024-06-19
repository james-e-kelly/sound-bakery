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

    public:
        sbk_id get_database_id() const;
        void set_database_id(sbk_id id);

        std::string_view get_database_name() const;
        void set_database_name(std::string_view name);

        bool get_editor_hidden() const { return editorHidden; }
        void set_editor_hidden(bool hidden) { editorHidden = hidden; }

        operator sbk_id() const { return m_objectID; }

        [[nodiscard]] MulticastDelegate<sbk_id, sbk_id> get_on_update_id();
        [[nodiscard]] MulticastDelegate<std::string_view, std::string_view> get_on_update_name();

    private:
        MulticastDelegate<sbk_id, sbk_id> m_onUpdateID;
        MulticastDelegate<std::string_view, std::string_view> m_onUpdateName;

        std::string m_objectName;
        sbk_id m_objectID = 0;

        bool editorHidden = false;  //< If true, the object won't render in the editor or be saved

        friend class database;
    };
}  // namespace sbk::core