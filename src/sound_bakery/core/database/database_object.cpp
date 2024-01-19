#include "database_object.h"

#include "sound_bakery/core/database/database.h"

using namespace SB::Core;

DatabaseObject::~DatabaseObject()
{
    // Database::get()->remove(this);
}

SB_ID SB::Core::DatabaseObject::getDatabaseID() const { return m_objectID; }

std::string_view SB::Core::DatabaseObject::getDatabaseName() const
{
    return m_objectName;
}

void SB::Core::DatabaseObject::setDatabaseID(SB_ID id)
{
    assert(m_objectID == 0 && "Shouldn't update an object's ID at runtime");

    Database::get()->addOrUpdateID(m_objectID, id, this);

    if (id != 0)
    {
        m_objectID = id;
    }
}

void SB::Core::DatabaseObject::setDatabaseName(std::string_view name)
{
    Database::get()->addOrUpdateName(m_objectName, name.data(), this);

    m_objectName = name;
}