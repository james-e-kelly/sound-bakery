#include "database_ptr.h"

#include "sound_bakery/core/database/database.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"

using namespace sbk::core;

namespace
{
    auto sbk::core::find_object(sbk_id id) -> concurrencpp::result<std::weak_ptr<database_object>>
    {
        co_await concurrencpp::resume_on(get_database_ptr_executor());

        if (const database* const objectOwner = sbk::engine::system::get())
        {
            co_return co_await objectOwner->try_find_database_object(id);
        }
        co_return std::weak_ptr<database_object>{};
    }

    auto SB_API object_id_is_child_of_parent(const sbk_id childToCheck, const sbk_id parent) -> concurrencpp::result<bool>
    {
        if (const sbk::core::database_ptr<database_object> parentPtr(parent); co_await parentPtr.lookup())
        {
            if (const std::shared_ptr<sbk::engine::node_base> nodeBase =
                    co_await parentPtr.shared_converted<sbk::engine::node_base>())
            {
                co_return nodeBase->hasChild(database_ptr<sbk::engine::node_base>(childToCheck));
            }
        }

        co_return false;
    }

    auto sbk::core::get_parent_id_from_id(const sbk_id id) -> concurrencpp::result<sbk_id>
    {
        if (id != 0)
        {
            if (const sbk::core::database_ptr<database_object> databasePtr(id); co_await databasePtr.lookup())
            {
                if (const std::shared_ptr<sbk::engine::node_base> nodeBase = co_await databasePtr.shared_converted<sbk::engine::node_base>())
                {
                    if (const sbk::engine::node_base* parent = nodeBase->get_parent())
                    {
                        co_return parent->get_database_id();
                    }
                }
            }
        }

        co_return 0;
    }

    auto sbk::core::get_database_ptr_executor() -> std::shared_ptr<concurrencpp::thread_pool_executor>
    {
        return sbk::engine::system::get()->get_thread_pool_executor();
    }
}