#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

std::weak_ptr<database_object> sbk::core::findObject(sbk_id id)
{
    if (database* const database = sbk::engine::system::get())
    {
        return database->try_find_weak(id);
    }
    return {};
}

bool sbk::core::objectIdIsChildOfParent(sbk_id childToCheck, sbk_id parent)
{
    sbk::core::DatabasePtr<database_object> parentPtr(parent);

    if (parentPtr.lookup())
    {
        if (sbk::engine::NodeBase* nodeBase = parentPtr->try_convert_object<sbk::engine::NodeBase>())
        {
            return nodeBase->hasChild(childToCheck);
        }
    }

    return false;
}

sbk_id sbk::core::getParentIdFromId(sbk_id id)
{
    if (id != 0)
    {
        const sbk::core::DatabasePtr<database_object> databasePtr(id);

        if (databasePtr.lookup())
        {
            if (sbk::engine::NodeBase* nodeBase = databasePtr->try_convert_object<sbk::engine::NodeBase>())
            {
                if (sbk::engine::NodeBase* parent = nodeBase->parent())
                {
                    return parent->get_database_id();
                }
            }
        }
    }

    return 0;
}