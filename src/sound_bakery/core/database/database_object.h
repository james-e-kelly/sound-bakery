#pragma once

#include "sound_bakery/core/object/object.h"

namespace SB::Core
{
    class SB_CLASS DatabaseObjectUtilities
    {
    public:
        virtual void onLoaded() {}
        virtual void onProjectLoaded() {}
        virtual void onDestroy() {}

        REGISTER_REFLECTION(DatabaseObjectUtilities)
    };

    /**
     * @brief Base object type for any object that can exist in the
     * editor/database. Holds an ID and name
     */
    class SB_CLASS DatabaseObject : public Object, public DatabaseObjectUtilities
    {
        REGISTER_REFLECTION(DatabaseObject, Object, DatabaseObjectUtilities)

    public:
        ~DatabaseObject();

        SB_ID getDatabaseID() const;
        void setDatabaseID(SB_ID id);

        std::string_view getDatabaseName() const;
        void setDatabaseName(std::string_view name);

        bool getEditorHidden() const { return editorHidden; }
        void setEditorHidden(bool hidden) { editorHidden = hidden; }

    public:
        operator SB_ID() const { return m_objectID; }

    private:
        std::string m_objectName;
        SB_ID m_objectID = 0;

        bool editorHidden = false; //< If true, the object won't render in the editor or be saved

        friend class Database;
    };
}  // namespace SB::Core