#include "database_object.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/system.h"

using namespace SB::Core;

DEFINE_REFLECTION(SB::Core::database_object)

sb_id SB::Core::database_object::get_database_id() const { return m_objectID; }

std::string_view SB::Core::database_object::get_database_name() const { return m_objectName; }

void SB::Core::database_object::set_database_id(sb_id id)
{
    assert(m_objectID == 0 && "Shouldn't update an object's ID at runtime");

    if (database* const database = SB::Engine::System::getDatabase())
    {
        database->add_or_update_id(m_objectID, id, this);
    }

    if (id != 0)
    {
        m_objectID = id;
    }
}

void SB::Core::database_object::set_database_name(std::string_view name)
{
    if (database* const database = SB::Engine::System::getDatabase())
    {
        database->add_or_update_name(m_objectName, name.data(), this);
    }

    m_objectName = name;

    m_debugName = name;
}