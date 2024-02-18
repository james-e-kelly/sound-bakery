#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"

using namespace SB::Core;

std::weak_ptr<DatabaseObject> SB::Core::findObject(SB_ID id)
{
    if (Database* database = Database::get())
    {
        return database->tryFindWeak(id);
    }
    return std::weak_ptr<DatabaseObject>();
}

bool SB::Core::objectIdIsChildOfParent(SB_ID childToCheck, SB_ID parent)
{
    SB::Core::DatabasePtr<DatabaseObject> parentPtr(parent);

    if (parentPtr.lookup())
    {
        if (SB::Engine::NodeBase* nodeBase = parentPtr->tryConvertObject<SB::Engine::NodeBase>())
        {
            return nodeBase->hasChild(childToCheck);
        }
    }

    return false;
}

SB_ID SB::Core::getParentIdFromId(SB_ID id)
{
    if (id != 0)
    {
        SB::Core::DatabasePtr<DatabaseObject> databasePtr(id);

        if (databasePtr.lookup())
        {
            if (SB::Engine::NodeBase* nodeBase = databasePtr->tryConvertObject<SB::Engine::NodeBase>())
            {
                if (SB::Engine::NodeBase* parent = nodeBase->parent())
                {
                    return parent->getDatabaseID();
                }
            }
        }
    }

    return 0;
}