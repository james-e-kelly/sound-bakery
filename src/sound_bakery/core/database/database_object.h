#pragma once

#include "sound_bakery/core/object/object.h"

namespace SB::Core
{
    class DatabaseObjectUtilities
    {
    public:
        virtual void onLoaded() {}
        virtual void onProjectLoaded() {}
        virtual void onDestroy() {}

        RTTR_ENABLE()
    };

    /**
     * @brief Base object type for any object that can exist in the
     * editor/database. Holds an ID and name
     */
    class DatabaseObject : public Object, public DatabaseObjectUtilities
    {
    public:
        ~DatabaseObject();

        SB_ID getDatabaseID() const;

        std::string_view getDatabaseName() const;

        void setDatabaseID(SB_ID id);

        void setDatabaseName(std::string_view name);

    public:
        operator SB_ID() const { return m_objectID; }

    private:
        std::string m_objectName;
        SB_ID m_objectID = 0;

        friend class Database;

        RTTR_ENABLE(Object, DatabaseObjectUtilities)
    };
}  // namespace SB::Core