#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/memory.h"
#include "sound_bakery/error/result.h"

namespace sbk::engine
{
    class system;
}

namespace sbk::core
{
    /**
     * @brief Creates, owns and tracks objects. Is owned by other object owners.
     *
     * This is the central location for object creation and management.
     * 
     * Through this, users can find the owners of objects and the owning sysytem object.
     */
    class SB_CLASS object_owner
    {
    public:
        virtual ~object_owner() = default;

        /**
         * @name Creation functions.
         */
        /**@{*/
        /**
         * @brief Create a raw object that is not globally tracked.
         *
         * Used to create simple objects like "normal" but still have them owned by this object.
         *
         * Useful for non-critical scenarios like creating widgets for a UI.
         */
        template <typename T>
        [[nodiscard]] auto create_raw_object() -> sbk::result<std::shared_ptr<T>>;

        /**
         * @brief Create an object that is fully owned by the caller.
         */
        template <typename T, typename... Args>
        [[nodiscard]] auto create_owned(sbk::memory::memory_resource& resource, Args&&... args) -> sbk::result<sbk::owned_ptr<T>>;

        /**
         * @brief Create an object that is owned by this owner.
         *
         * The object is tracked by the object_tracker (likely the system object) and is therefore searchable later.
         *
         * Used for keeping count of our objects, useful for types like game objects.
         */
        [[nodiscard]] auto create_runtime_object(const rttr::type& type) -> sbk::result<std::shared_ptr<object>>;

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
        [[nodiscard]] auto create_database_object(const rttr::type& type, bool addToDatabase = true) -> sbk::result<std::shared_ptr<database_object>>;

        /**
         * @brief Templated version of create_runtime_object.
         */
        template <typename T>
        [[nodiscard]] auto create_runtime_object() -> sbk::result<std::shared_ptr<T>>;

        /**
         * @brief Templated version of create_database_object.
         */
        template <typename T>
        [[nodiscard]] auto create_database_object(bool addToDatabase = true) -> sbk::result<std::shared_ptr<T>>;
        /**@}*/

        [[nodiscard]] auto get_owner() const -> object_owner* { return m_owner; }
        [[nodiscard]] auto get_system() const noexcept -> sbk::engine::system*;

        /**
         * @brief Track an already existing object.
         */
        auto add_reference_to_object(std::shared_ptr<database_object>& object) -> void;

        /**
         * @brief Remove the reference to this object, if we have one.
         *
         * The object is not guaranteed to be destroyed as other objects might still hold references to it.
         */
        auto remove_object(const std::shared_ptr<object>& object) -> std::vector<std::shared_ptr<sbk::core::object>>::iterator;

        /**
         * @brief Remove all references.
         *
         * The objects are not guaranteed to be destroyed as other objects might still hold references to it.
         */
        auto remove_all() -> void;

        [[nodiscard]] auto get_objects() -> std::vector<std::shared_ptr<object>>&;
        [[nodiscard]] auto get_objects() const -> const std::vector<std::shared_ptr<object>>&;
        [[nodiscard]] auto get_objects_size() const -> std::size_t;  //< Get number of objects this object owns

    protected:
        auto set_owner(object_owner* newOwner) -> void;

    private:
        object_owner* m_owner = nullptr;
        std::vector<std::shared_ptr<object>> m_objects;
    };

    template <typename T>
    auto object_owner::create_raw_object() -> sbk::result<std::shared_ptr<T>>
    {
        ZoneScoped;
        std::shared_ptr<T> result = std::make_shared<T>();

        m_objects.emplace_back(result);

        result->set_owner(this);
        result->cache_type();

        return result;
    }

    template <typename T, typename... Args>
    auto object_owner::create_owned(sbk::memory::memory_resource& resource, Args&&... args) -> sbk::result<sbk::owned_ptr<T>>
    {
        static_assert(std::is_base_of_v<object_owner, T>, "create_owned is for graph nodes; they must derive from object_owner");

        ZoneScoped;

        void* const memory = resource.allocate(sizeof(T), alignof(T));
        SBK_CHECK(memory != nullptr, SBK_ERR_OUT_OF_MEMORY);

        try
        {
            T* const raw = ::new (memory) T(std::forward<Args>(args)...);
            raw->set_owner(this); // owned objects still know their parent/owner
            return sbk::owned_ptr<T>(raw, sbk::memory::owned_object_deleter<T>(resource));
        }
        catch (const std::exception& exception)
        {
            // Constructor threw: the placement-new never completed, so return the raw
            // block ourselves -- no destructor to run. Mirrors create_runtime_object.
            resource.deallocate(memory, sizeof(T), alignof(T));
            return sbk::make_error(SBK_ERR_OUT_OF_MEMORY, exception.what());
        }
    }

    template <typename T>
    auto object_owner::create_runtime_object() -> sbk::result<std::shared_ptr<T>>
    {
        ZoneScoped;
        static_assert(std::is_base_of_v<sbk::core::object, T>, "Runtime objects must derive from sbk::core::object");

        SBK_TRY(auto object, create_runtime_object(rttr::type::get<T>()));

        if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
        {
            return castedObject;
        }

        return sbk::make_error(SBK_ERR_BAKERY);
    }

    template <typename T>
    auto object_owner::create_database_object(bool addToDatabase) -> sbk::result<std::shared_ptr<T>>
    {
        ZoneScoped;
        static_assert(std::is_base_of_v<sbk::core::database_object, T>, "Database objects must derive from sbk::core::database_object");

        SBK_TRY(auto object, create_database_object(rttr::type::get<T>(), addToDatabase));

        if (std::shared_ptr<T> castedObject = std::static_pointer_cast<T>(object))
        {
            return castedObject;
        }

        return sbk::make_error(SBK_ERR_BAKERY);
    }
}  // namespace sbk::core
