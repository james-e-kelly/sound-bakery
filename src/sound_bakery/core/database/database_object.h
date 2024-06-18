#pragma once

#include "sound_bakery/core/object/object.h"

namespace sbk::core
{
    class SB_CLASS database_object_utilities
    {
    public:
        virtual void onLoaded() {}
        virtual void onProjectLoaded() {}
        virtual void onDestroy() {}

        REGISTER_REFLECTION(database_object_utilities)
    };

    /**
     * @brief Base object type for any object that can exist in the
     * editor/database. Holds an ID and name
     */
    class SB_CLASS database_object : public object, public database_object_utilities
    {
        REGISTER_REFLECTION(database_object, object, database_object_utilities)

    public:
        sbk_id get_database_id() const;
        void set_database_id(sbk_id id);

        std::string_view get_database_name() const;
        void set_database_name(std::string_view name);

        bool get_editor_hidden() const { return editorHidden; }
        void set_editor_hidden(bool hidden) { editorHidden = hidden; }

        operator sbk_id() const { return m_objectID; }

    private:
        std::string m_objectName;
        sbk_id m_objectID = 0;

        bool editorHidden = false;  //< If true, the object won't render in the editor or be saved

        friend class database;
    };
}  // namespace sbk::core