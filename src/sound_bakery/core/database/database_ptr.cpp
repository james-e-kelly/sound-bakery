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