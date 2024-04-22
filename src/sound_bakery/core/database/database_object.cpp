#include "database_object.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"

using namespace SB::Core;

DEFINE_REFLECTION(SB::Core::DatabaseObject)

DatabaseObject::~DatabaseObject()
{
    // Database::get()->remove(this);
}

SB_ID SB::Core::DatabaseObject::getDatabaseID() const { return m_objectID; }

std::string_view SB::Core::DatabaseObject::getDatabaseName() const { return m_objectName; }

void SB::Core::DatabaseObject::setDatabaseID(SB_ID id)
{
    assert(m_objectID == 0 && "Shouldn't update an object's ID at runtime");

    if (Database* const database = SB::Engine::System::getDatabase())
    {
        database->addOrUpdateID(m_objectID, id, this);
    }

    if (id != 0)
    {
        m_objectID = id;
    }
}

void SB::Core::DatabaseObject::setDatabaseName(std::string_view name)
{
    if (Database* const database = SB::Engine::System::getDatabase())
    {
        database->addOrUpdateName(m_objectName, name.data(), this);
    }

    m_objectName = name;

    m_debugName = name;
}