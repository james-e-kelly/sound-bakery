#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

using namespace SB::Core;

std::weak_ptr<database_object> SB::Core::findObject(sb_id id)
{
    if (database* const database = SB::Engine::System::getDatabase())
    {
        return database->try_find_weak(id);
    }
    return std::weak_ptr<database_object>();
}

bool SB::Core::objectIdIsChildOfParent(sb_id childToCheck, sb_id parent)
{
    SB::Core::DatabasePtr<database_object> parentPtr(parent);

    if (parentPtr.lookup())
    {
        if (SB::Engine::NodeBase* nodeBase = parentPtr->try_convert_object<SB::Engine::NodeBase>())
        {
            return nodeBase->hasChild(childToCheck);
        }
    }

    return false;
}

sb_id SB::Core::getParentIdFromId(sb_id id)
{
    if (id != 0)
    {
        SB::Core::DatabasePtr<database_object> databasePtr(id);

        if (databasePtr.lookup())
        {
            if (SB::Engine::NodeBase* nodeBase = databasePtr->try_convert_object<SB::Engine::NodeBase>())
            {
                if (SB::Engine::NodeBase* parent = nodeBase->parent())
                {
                    return parent->get_database_id();
                }
            }
        }
    }

    return 0;
}