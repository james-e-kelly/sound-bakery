#pragma once

#include "sound_bakery/pch.h"

namespace sbk::core
{
    enum class object_iterate_action
    {
        next,               //< Continue iterating and go to the next object
        destroy_and_next,            //< Destroy this object and keep iterating
        destroy_and_stop,   //< Destroy this object and stop iterating
        stop                //< Stop iterating
    };

    /**
     * @brief Creates, owns and tracks objects.
     *
     * This is the central location for object creation.
     */
    class SB_CLASS object_owner
    {
    public:
        /**
         * @brief Create a raw object that is not globally tracked.
         * 
         * Used to create simple objects like "normal" but still have them owned by this object.
         * 
         * Useful for non-critical scenarios like creating widgets for a UI.
         */
        template <typename T>
        [[nodiscard]] auto create_raw_object() -> concurrencpp::result<std::shared_ptr<T>>;

        /**
         * @brief Create an object that is owned by this owner.
         * 
         * The object is tracked by the object_tracker (likely the system object) and is therefore searchable later.
         * 
         * Used for keeping count of our objects, useful for types like game objects.
         */
        [[nodiscard]] auto create_runtime_object(const rttr::type& type) -> concurrencpp::result<std::shared_ptr<object>>;

        /**
         * @brief Create an object that derives from database_object. 
         * 
         * The object can be optionally tracked by the database object.
         * 
         * All database_object creation should go through this method.
         * 
         * @param addToDatabase to automatically track the object. If set to false, the user is responsible for adding
         * the object to the database.
         */
        [[nodiscard]] auto create_database_object(const rttr::type& type, bool addToDatabase = true) -> concurrencpp::result<std::shared_ptr<database_object>>;

        /**
         * @brief Templated version of create_runtime_object.
         */
        template <typename T>
        [[nodiscard]] auto create_runtime_object() -> concurrencpp::result<std::shared_ptr<T>>;

        /**
         * @brief Templated version of create_database_object.
         */
        template <typename T>
        [[nodiscard]] auto create_database_object(bool addToDatabase = true) -> concurrencpp::result<std::shared_ptr<T>>;

        /**
         * @brief Track an already existing object.
         */
        auto add_reference_to_object(std::shared_ptr<object> object) -> concurrencpp::result<void>;

        /**
         * @brief Remove the reference to this object, if we have one.
         * 
         * The object is not guaranteed to be destroyed as other objects might still hold references to it.
         */
        auto remove_reference_to_object(std::shared_ptr<object> object) -> concurrencpp::result<void>;

        /**
         * @brief Remove all references.
         * 
         * The objects are not guaranteed to be destroyed as other objects might still hold references to it.
         */
        auto remove_all() -> concurrencpp::result<void>;

        /**
         * @brief Iterate over the owning/referenced objects.
         * @param function to call on each object. Likely to be a lambda. The function should return a object_iterate_action to tell the loop what to do with the object.
         * @remark Acquires the object lock and iterates in a thread-safe way.
         */
        template<class Function>
        auto iterate_referenced_objects(Function function) -> concurrencpp::result<void>
        {
            const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();

            for (auto iter = m_objects.begin(); iter != m_objects.end(); ++iter)
            {
                const object_iterate_action result = function(*iter);

                switch (result)
                {
                    case object_iterate_action::destroy_and_next: 
                    { 
                        iter = m_objects.erase(iter);
                        break; 
                    }
                    case object_iterate_action::destroy_and_stop: 
                    {
                        m_objects.erase(iter);
                        co_return;
                    }
                    case object_iterate_action::stop: 
                    { 
                        co_return;
                    }

                    case object_iterate_action::next:
                    default:
                        break;
                }

                if (iter == m_objects.end())
                {
                    co_return;
                }
            }
        }

        /**
         * @brief Iterate over the const owning/referenced objects.
         * @param function function to call on each object. The function should return a object_iterate_action to tell the loop what to do with the object. However, any destroy commands will be ignored.
         */
        template <class Function>
        auto iterate_const_referenced_objects(Function function) const -> concurrencpp::result<void>
        {
            const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();

            for (auto iter = m_objects.begin(); iter != m_objects.end(); ++iter)
            {
                const object_iterate_action result = function(*iter);

                switch (result)
                {
                    case object_iterate_action::destroy_and_stop:
                    case object_iterate_action::stop:
                    {
                        co_return;
                    }
                    case object_iterate_action::destroy_and_next:
                    case object_iterate_action::next:
                    default:
                        break;
                }
            }
        }

        template<class Function>
        [[nodiscard]] auto referenced_object_exists_predicate(Function function) const -> concurrencpp::result<bool>
        {
            const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();
            co_return std::find_if(m_objects.cbegin(), m_objects.cend(), std::move(function)) != m_objects.cend();
        }

        [[nodiscard]] auto get_referenced_objects() const -> concurrencpp::result<std::vector<std::shared_ptr<object>>>;
        [[nodiscard]] auto get_referenced_object_at(std::size_t index) const -> concurrencpp::result<std::shared_ptr<object>>;
        [[nodiscard]] auto get_referenced_objects_size() const -> concurrencpp::result<std::size_t>; //< Get number of objects this object owns

    private:
        auto get_object_lock() const -> concurrencpp::lazy_result<concurrencpp::scoped_async_lock>;

        mutable concurrencpp::async_lock m_objectsLock;
        std::vector<std::shared_ptr<object>> m_objects;
    };

#include "object_owner.inl"
}  // namespace sbk::core
