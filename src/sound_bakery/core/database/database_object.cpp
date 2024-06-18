#include "database_object.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

DEFINE_REFLECTION(sbk::core::database_object)

sbk_id sbk::core::database_object::get_database_id() const { return m_objectID; }

std::string_view sbk::core::database_object::get_database_name() const { return m_objectName; }

void sbk::core::database_object::set_database_id(sbk_id id)
{
    assert(m_objectID == 0 && "Shouldn't update an object's ID at runtime");

    if (database* const database = sbk::engine::system::get())
    {
        database->add_or_update_id(m_objectID, id, this);
    }

    if (id != 0)
    {
        m_objectID = id;
    }
}

void sbk::core::database_object::set_database_name(std::string_view name)
{
    if (database* const database = sbk::engine::system::get())
    {
        database->add_or_update_name(m_objectName, this, name.data());
    }

    m_objectName = name;

    m_debugName = name;
}