#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/error/result.h"

namespace sbk::core
{
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
        [[nodiscard]] auto create_raw_object() -> sbk::result<std::shared_ptr<T>>;

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
        [[nodiscard]] auto get_objects_size() const -> std::size_t; //< Get number of objects this object owns

    private:
        std::vector<std::shared_ptr<object>> m_objects;
    };

#include "object_owner.inl"
}  // namespace sbk::core
