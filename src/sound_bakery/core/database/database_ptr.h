#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/database/database_object.h"

#include <compare>

namespace sbk::core
{
    class database_object;

    auto SB_API find_object(sbk_id id) -> std::weak_ptr<database_object>;
    auto SB_API object_id_is_child_of_parent(sbk_id childToCheck, sbk_id parent) -> bool;
    auto SB_API get_parent_id_from_id(sbk_id id) -> sbk_id;

    /** Lazy Pointer
     * Lazy pointers store an Indentifier to an object and use it to find the
     * object later Lazy pointers can be serialized so references can persist
     * through multiple shutdowns Passing only an indentifier will create a
     * pointer that references no object but will try to find the object when
     * accessing the object
     */
    template <typename TObject>
    class database_ptr
    {
    public:
        using TThisType       = database_ptr<TObject>;
        using TIdentifierType = sbk_id;
        using TObjectPtr      = TObject*;
        using TObjectRef      = TObject&;
        using TObjectWeak     = std::weak_ptr<database_object>;
        using TObjectShared   = std::shared_ptr<TObject>;
        using TPtrType        = std::weak_ptr<database_object>;

        static_assert(!std::is_pointer<TObject>::value);

    public:
        /**
         * @brief Creates an empty and null LazyPtr
         */
        database_ptr() : m_objectID(), m_objectPtr(), m_null(true) {}

        database_ptr(const TThisType& other) = default;
        database_ptr(TThisType&& other)      = default;

        /**
         * @brief Creates a LazyPtr that can lookup its object pointer after
         * construction
         * @param id ID of the object to reference
         */
        database_ptr(sbk_id id) : m_objectID(id), m_objectPtr(), m_null(true) {}

        /**
         * @brief Create a valid LazyPtr
         * @param object
         */
        database_ptr(const TObjectShared& object)
            : m_objectID(object ? static_cast<TIdentifierType>(*object) : 0),
              m_objectPtr(object),
              m_null(object == nullptr)
        {
        }

        /**
         * @brief Create a valid LazyPtr
         * @param object
         */
        database_ptr(const TObjectPtr& object)
            : m_objectID(object ? static_cast<TIdentifierType>(*object) : 0),
              m_objectPtr(find_object(id())),
              m_null(object == nullptr)
        {
        }

        /**
         * @brief Create an empty and null LazyPtr
         * @param nullptr_t
         */
        database_ptr(std::nullptr_t) : m_objectID(), m_objectPtr(), m_null(true) {}

        ~database_ptr() = default;

    public:
        /**
         * @brief Get ID of the referenced object
         */
        [[nodiscard]] auto id() const noexcept -> sbk_id { return m_objectID; }

        [[nodiscard]] auto shared() const noexcept -> TObjectShared
        {
            lookup();

            if (m_objectPtr.expired())
            {
                return std::shared_ptr<TObject>();
            }
            return std::static_pointer_cast<TObject>(m_objectPtr.lock());
        }

        [[nodiscard]] auto weak() const noexcept -> TObjectWeak { return m_objectPtr; }

        /**
         * @brief Returns true if we hold a valid ID and can search for an
         * object at runtime
         */
        [[nodiscard]] auto has_id() const noexcept -> bool { return m_objectID != TIdentifierType(); }

        /**
         * @brief Returns true if the object pointer is not set
         */
        [[nodiscard]] auto null() const noexcept -> bool { return m_null || m_objectPtr.expired(); }

        /**
         * @brief Returns true if we hold an ID but haven't found the live
         * object to point to yet
         */
        [[nodiscard]] auto pending() const noexcept -> bool { return has_id() && null(); }

        /**
         * @brief Returns true if we previously referenced an object that has
         * been destroyed
         */
        [[nodiscard]] auto stale() const noexcept -> bool { return !m_null && m_objectPtr.expired(); }

        /**
         * @brief Returns true if we hold an ID and a valid pointer to the
         * object
         */
        [[nodiscard]] auto valid() const noexcept -> bool { return has_id() && !null(); }

        /**
         * @brief Find the live object referenced by the ID and store it
         * @return true if the object was found and we're a valid ptr
         */
        auto lookup() const noexcept -> bool
        {
            if (pending())
            {
                m_objectPtr = find_object(id());
                m_null      = m_objectPtr.expired();
            }
            return valid();
        }

        /**
         * @brief Clear all references
         */
        auto reset(TObjectPtr object = nullptr) -> void
        {
            m_objectID = object ? static_cast<TIdentifierType>(*object) : TIdentifierType();
            m_objectPtr.reset();
            m_null = true;

            lookup();
        }

        auto reset(const TObjectShared& object) -> void
        {
            m_objectID  = object ? static_cast<TIdentifierType>(*object.get()) : TIdentifierType();
            m_objectPtr = object;
            m_null      = m_objectPtr.expired();
        }

        auto operator=(TObjectShared object) -> TThisType&
        {
            // Not happy about the lock here
            if (m_objectPtr.lock() != object)
            {
                reset(object);
            }
            return *this;
        }

        /**
         * @brief Assign this LazyPtr to a new object, potentially destroying
         * the current object if we're acting as a UniquePtr
         * @param object to assign to
         * @return this
         */
        auto operator=(TObjectPtr object) -> TThisType&
        {
            reset(object);
            return *this;
        }

        auto operator=(const TThisType& other) noexcept -> TThisType&
        {
            if (this != &other && m_objectID != other.id())
            {
                m_objectID  = other.id();
                m_objectPtr = other.weak();
                m_null      = other.null();
            }

            return *this;
        }

        auto operator=(TThisType&& other) noexcept -> TThisType&
        {
            if (id() != other.id())
            {
                m_objectID  = other.id();
                m_objectPtr = std::move(other.m_objectPtr);
                m_null      = other.null();
            }

            other.m_objectID = TIdentifierType();
            other.m_objectPtr.reset();
            other.m_null = true;

            return *this;
        }

        /**
         * @brief Returns true if this LazyPtr references a valid object
         */
        operator bool() const { return valid(); }

        /**
         * @brief Returns true if this LazyPtr is invalid
         */
        auto operator!() const -> bool { return !valid(); }

    protected:
        sbk_id m_objectID;
        mutable TPtrType m_objectPtr = TPtrType();
        mutable bool m_null;
    };

    template <typename T1, typename T2>
    auto operator<=>(const database_ptr<T1>& lhs, const database_ptr<T2>& rhs)
    {
        return lhs.id() <=> rhs.id();
    }

    /**
     * @brief Compare a LazyPtr with a pointer for equality
     * @return true if the LazyPtr references the object
     */
    template <typename T>
    auto operator==(const database_ptr<T>& lhs, const T* rhs) -> bool
    {
        return lhs.raw() == rhs;
    }

    /**
     * @brief Syntactic type to define a pointer that must be a child of the
     * owning object.
     */
    template <typename TObject>
    class child_ptr final : public database_ptr<TObject>
    {
    public:
        using TThisType = child_ptr<TObject>;

    public:
        /**
         * @brief Default constructor is exposed for RTTR but not for the user.
         *
         * @warning Child Ptr objects must belong to a @ref DatabaseObject at
         * construction time.
         */
        child_ptr() = default;

        child_ptr(const TThisType& other) : database_ptr<TObject>(other), m_ownerID(other.m_ownerID)
        {
            // If we don't have an owner, try finding it now
            // We can't do any other checks because we were empty before this copy
            // Because we can't do checks, we're just hoping the passed in object is valid
            if (m_ownerID == 0)
            {
                m_ownerID = get_parent_id_from_id(other.m_objectID);
            }
        }

        child_ptr(TThisType&& other) = default;
        ~child_ptr()                 = default;

        /**
         * @brief Construct a new Child Ptr object with an owner.
         *
         * Child Ptr types must have an owner so it can check whether an
         * assigned ptr is a child or not.
         *
         * @param owner to check for child objects on
         */
        child_ptr(const database_object& owner) : database_ptr<TObject>(), m_ownerID(owner.get_database_id()) {}

        /**
         * @brief Construct a new child_ptr that points to the ID.
         *
         * Tries to find the owner from the ID.
         */
        child_ptr(sbk_id id) : database_ptr<TObject>(id), m_ownerID(get_parent_id_from_id(id)) {}

        auto operator=(typename database_ptr<TObject>::TIdentifierType id) -> TThisType&
        {
            set_id(id);

            return *this;
        }

        auto operator=(typename database_ptr<TObject>::TObjectPtr object) -> TThisType&
        {
            reset(object);

            return *this;
        }

        auto operator=(const TThisType& other) -> TThisType&
        {
            if (this != &other && database_ptr<TObject>::id() != other.id())
            {
                if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
                {
                    m_ownerID = get_parent_id_from_id(database_ptr<TObject>::m_objectID);
                }

                // If we don't have an owner, we don't care about checking children
                // We can completely copy other
                if (m_ownerID == 0)
                {
                    database_ptr<TObject>::m_objectID  = other.id();
                    database_ptr<TObject>::m_objectPtr = other.weak();
                    database_ptr<TObject>::m_null      = other.null();
                    m_ownerID                          = other.m_ownerID;
                }
                // If owner isn't trying to be changed, we can just check children and update the pointed to ID
                else if (m_ownerID == other.m_ownerID || other.m_ownerID == 0)
                {
                    // Do child check
                    if (object_id_is_child_of_parent(other.m_objectID, m_ownerID))
                    {
                        database_ptr<TObject>::m_objectID  = other.id();
                        database_ptr<TObject>::m_objectPtr = other.weak();
                        database_ptr<TObject>::m_null      = other.null();
                    }
                }
                // else: don't allow changing owner IDs once they're set
            }

            return *this;
        }

        auto set_id(typename database_ptr<TObject>::TIdentifierType id = 0) -> void
        {
            // Fill our get_parent ID if we didn't have it already
            if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
            {
                m_ownerID = get_parent_id_from_id(database_ptr<TObject>::m_objectID);
            }

            if (id == 0)
            {
                database_ptr<TObject>::m_objectID = 0;
                database_ptr<TObject>::m_objectPtr.reset();
                database_ptr<TObject>::m_null = true;
            }
            else
            {
                if (m_ownerID == 0 || object_id_is_child_of_parent(id, m_ownerID))
                {
                    database_ptr<TObject>::m_objectID = id;
                    database_ptr<TObject>::m_objectPtr.reset();
                    database_ptr<TObject>::m_null = true;
                }
            }
        }

        auto reset(typename database_ptr<TObject>::TObjectPtr object = nullptr) -> void
        {
            // Fill our get_parent ID if we didn't have it already
            if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
            {
                m_ownerID = get_parent_id_from_id(database_ptr<TObject>::m_objectID);
            }

            // Reset pointed to values but retain the owner ID
            if (object == nullptr)
            {
                database_ptr<TObject>::m_objectID = 0;
                database_ptr<TObject>::m_objectPtr.reset();
                database_ptr<TObject>::m_null = true;
            }
            // Point to new object if it's a child of our owner
            else
            {
                sbk_id newObjectID = static_cast<typename database_ptr<TObject>::TIdentifierType>(*object);

                if (m_ownerID == 0 || object_id_is_child_of_parent(newObjectID, m_ownerID))
                {
                    database_ptr<TObject>::m_objectID = newObjectID;
                    database_ptr<TObject>::m_objectPtr.reset();
                    database_ptr<TObject>::m_null = true;
                }
            }
        }

    private:
        typename database_ptr<TObject>::TIdentifierType m_ownerID = 0;
    };
}  // namespace sbk::core

namespace std
{
    template <typename T>
    struct hash<sbk::core::database_ptr<T>>
    {
        auto operator()(const sbk::core::database_ptr<T>& k) const -> size_t { return hash<sbk_id>{}(k.id()); }
    };

    template <typename T>
    struct hash<sbk::core::child_ptr<T>>
    {
        auto operator()(const sbk::core::child_ptr<T>& k) const -> size_t { return hash<sbk_id>{}(k.id()); }
    };
}  // namespace std

#include <rttr/wrapper_mapper.h>

namespace rttr
{
    template <typename T>
    struct wrapper_mapper<sbk::core::child_ptr<T>>
    {
        using wrapped_type = decltype(sbk::core::child_ptr<T>(0).id());
        using type         = sbk::core::child_ptr<T>;

        inline static auto get(const type& obj) -> wrapped_type { return obj.id(); }

        inline static auto create(const wrapped_type& t) -> type { return type(t); }

        template <typename T2>
        inline static auto convert(const type& source, bool& ok) -> sbk::core::child_ptr<T2>
        {
            sbk::core::child_ptr<T2> convertedLazyPtr(source.id());

            ok = source.has_id() == convertedLazyPtr.has_id();

            return convertedLazyPtr;
        }
    };

    template <typename T>
    struct wrapper_mapper<sbk::core::database_ptr<T>>
    {
        using wrapped_type = decltype(sbk::core::database_ptr<T>().id());
        using type         = sbk::core::database_ptr<T>;

        inline static auto get(const type& obj) -> wrapped_type { return obj.id(); }

        inline static auto create(const wrapped_type& t) -> type { return type(t); }

        template <typename T2>
        inline static auto convert(const type& source, bool& ok) -> sbk::core::database_ptr<T2>
        {
            sbk::core::database_ptr<T2> convertedLazyPtr(source.id());

            ok = source.has_id() == convertedLazyPtr.has_id();

            return convertedLazyPtr;
        }
    };

}  // namespace rttr
