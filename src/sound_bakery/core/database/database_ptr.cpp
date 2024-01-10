#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"

using namespace SB::Core;

std::weak_ptr<DatabaseObject> SB::Core::findObject(SB_ID id)
{
	if (Database* database = Database::get())
	{
		return database->tryFindWeak(id);
	}
	return std::weak_ptr<DatabaseObject>();
}
