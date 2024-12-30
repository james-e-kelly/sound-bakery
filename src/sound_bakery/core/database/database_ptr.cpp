#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

std::weak_ptr<database_object> sbk::core::find_object(sbk_id id)
{
    if (const database* const objectOwner = sbk::engine::system::get())
    {
        return objectOwner->try_find(id);
    }
    return {};
}

bool sbk::core::object_id_is_child_of_parent(sbk_id childToCheck, sbk_id parent)
{
    sbk::core::database_ptr<database_object> parentPtr(parent);

    if (parentPtr.lookup())
    {
        if (sbk::engine::node_base* nodeBase = parentPtr->try_convert_object<sbk::engine::node_base>())
        {
            return nodeBase->hasChild(childToCheck);
        }
    }

    return false;
}

sbk_id sbk::core::get_parent_id_from_id(sbk_id id)
{
    if (id != 0)
    {
        const sbk::core::database_ptr<database_object> databasePtr(id);

        if (databasePtr.lookup())
        {
            if (sbk::engine::node_base* nodeBase = databasePtr->try_convert_object<sbk::engine::node_base>())
            {
                if (sbk::engine::node_base* parent = nodeBase->get_parent())
                {
                    return parent->get_database_id();
                }
            }
        }
    }

    return 0;
}