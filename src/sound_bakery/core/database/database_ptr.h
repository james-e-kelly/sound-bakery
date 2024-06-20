#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/pch.h"

namespace sbk::core
{
    class database_object;

    std::weak_ptr<database_object> SB_API findObject(sbk_id id);
    bool SB_API objectIdIsChildOfParent(sbk_id childToCheck, sbk_id parent);
    sbk_id SB_API getParentIdFromId(sbk_id id);

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
              m_null(object.use_count() == 0)
        {
        }

        /**
         * @brief Create a valid LazyPtr
         * @param object
         */
        database_ptr(const TObjectPtr& object)
            : m_objectID(static_cast<TIdentifierType>(*object)), m_objectPtr(findObject(id())), m_null(false)
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
         * @return
         */
        sbk_id id() const noexcept { return m_objectID; }

        TObjectShared shared() const noexcept
        {
            lookup();

            if (m_objectPtr.expired())
            {
                return std::shared_ptr<TObject>();
            }
            else
            {
                return std::static_pointer_cast<TObject>(m_objectPtr.lock());
            }
        }

        TObjectWeak weak() const noexcept { return m_objectPtr; }

        /**
         * @brief Get raw pointer of the referenced object
         * @return
         */
        TObjectPtr raw() const noexcept { return shared().get(); }

        TObjectPtr lookupRaw() const noexcept
        {
            lookup();
            return raw();
        }

        /**
         * @brief Returns true if we hold a valid ID and can search for an
         * object at runtime
         * @return
         */
        bool hasId() const { return m_objectID != TIdentifierType(); }

        /**
         * @brief Returns true if the object pointer is not set
         * @return
         */
        bool null() const { return m_null || m_objectPtr.expired(); }

        /**
         * @brief Returns true if we hold an ID but haven't found the live
         * object to point to yet
         * @return
         */
        bool pending() const { return hasId() && null(); }

        /**
         * @brief Returns true if we previously referenced an object that has
         * been destroyed
         * @return
         */
        bool stale() const { return !m_null && m_objectPtr.expired(); }

        /**
         * @brief Returns true if we hold an ID and a valid pointer to the
         * object
         * @return
         */
        bool valid() const { return hasId() && !null(); }

    public:
        /**
         * @brief Find the live object referenced by the ID and store it
         * @return true if the object was found and we're a valid ptr
         */
        bool lookup() const
        {
            if (pending())
            {
                m_objectPtr = findObject(id());
                m_null      = m_objectPtr.expired();
            }
            return valid();
        }

        /**
         * @brief Clear all references
         */
        void reset(TObjectPtr object = nullptr)
        {
            m_objectID = object ? static_cast<TIdentifierType>(*object) : TIdentifierType();
            m_objectPtr.reset();
            m_null = true;

            lookup();
        }

    public:
        TThisType& operator=(TObjectShared object)
        {
            if (raw() != object.get())
            {
                reset(object.get());
            }
            return *this;
        }

        /**
         * @brief Assign this LazyPtr to a new object, potentially destroying
         * the current object if we're acting as a UniquePtr
         * @param object to assign to
         * @return this
         */
        TThisType& operator=(TObjectPtr object)
        {
            if (raw() != object)
            {
                reset(object);
            }
            return *this;
        }

        TThisType& operator=(const TThisType& other)
        {
            if (id() != other.id())
            {
                m_objectID  = other.id();
                m_objectPtr = other.weak();
                m_null      = other.null();
            }

            return *this;
        }

        TThisType& operator=(const TThisType&& other)
        {
            if (id() != other.id())
            {
                m_objectID  = other.id();
                m_objectPtr = other.weak();
                m_null      = other.null();
            }

            return *this;
        }

        /**
         * @brief Returns true if this LazyPtr references a valid object
         */
        operator bool() const { return valid(); }

        /**
         * @brief Returns true if this LazyPtr is invalid
         */
        bool operator!() const { return !valid(); }

        /**
         * @brief Access the raw object
         * @return raw object
         */
        TObjectPtr operator->() const { return raw(); }

    protected:
        sbk_id m_objectID;
        mutable TPtrType m_objectPtr = TPtrType();
        mutable bool m_null;
    };

    /**
     * @brief Compare any two LazyPtr's IDs for equality
     * @tparam T1 type 1
     * @tparam T2 type 2
     * @tparam U1 Unique 1
     * @tparam U2 Unique 2
     * @param lhs first LazyPtr to compare
     * @param rhs second LazyPtr to compare
     * @return true if the two LazyPtrs share the same ID
     */
    template <typename T1, typename T2>
    bool operator==(const database_ptr<T1>& lhs, const database_ptr<T2>& rhs)
    {
        return lhs.id() == rhs.id();
    }

    /**
     * @brief Compare a LazyPtr with a pointer for equality
     * @tparam T
     * @tparam U
     * @param lhs
     * @param rhs
     * @return true if the LazyPtr references the object
     */
    template <typename T>
    bool operator==(const database_ptr<T>& lhs, const T* rhs)
    {
        return lhs.raw() == rhs;
    }

    /**
     * @brief Compares the lhs ID with the rhs ID
     * @tparam T1
     * @tparam T2
     * @tparam U1
     * @tparam U2
     * @param lhs
     * @param rhs
     * @return
     */
    template <typename T1, typename T2>
    bool operator<(const database_ptr<T1>& lhs, const database_ptr<T2>& rhs)
    {
        return lhs.id() < rhs.id();
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
                m_ownerID = getParentIdFromId(other.m_objectID);
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
        child_ptr(sbk_id id) : database_ptr<TObject>(id), m_ownerID(getParentIdFromId(id)) {}

        TThisType& operator=(typename database_ptr<TObject>::TIdentifierType id)
        {
            setID(id);

            return *this;
        }

        TThisType& operator=(typename database_ptr<TObject>::TObjectPtr object)
        {
            reset(object);

            return *this;
        }

        TThisType& operator=(const TThisType& other)
        {
            if (database_ptr<TObject>::id() != other.id())
            {
                if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
                {
                    m_ownerID = getParentIdFromId(database_ptr<TObject>::m_objectID);
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
                    if (objectIdIsChildOfParent(other.m_objectID, m_ownerID))
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

        void setID(typename database_ptr<TObject>::TIdentifierType id = 0)
        {
            // Fill our parent ID if we didn't have it already
            if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
            {
                m_ownerID = getParentIdFromId(database_ptr<TObject>::m_objectID);
            }

            if (id == 0)
            {
                database_ptr<TObject>::m_objectID = 0;
                database_ptr<TObject>::m_objectPtr.reset();
                database_ptr<TObject>::m_null = true;
            }
            else
            {
                if (m_ownerID == 0 || objectIdIsChildOfParent(id, m_ownerID))
                {
                    database_ptr<TObject>::m_objectID = id;
                    database_ptr<TObject>::m_objectPtr.reset();
                    database_ptr<TObject>::m_null = true;
                }
            }
        }

        void reset(typename database_ptr<TObject>::TObjectPtr object = nullptr)
        {
            // Fill our parent ID if we didn't have it already
            if (m_ownerID == 0 && database_ptr<TObject>::m_objectID != 0)
            {
                m_ownerID = getParentIdFromId(database_ptr<TObject>::m_objectID);
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

                if (m_ownerID == 0 || objectIdIsChildOfParent(newObjectID, m_ownerID))
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
        size_t operator()(const sbk::core::database_ptr<T>& k) const { return hash<sbk_id>{}(k.id()); }
    };

    template <typename T>
    struct hash<sbk::core::child_ptr<T>>
    {
        size_t operator()(const sbk::core::child_ptr<T>& k) const { return hash<sbk_id>{}(k.id()); }
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

        inline static wrapped_type get(const type& obj) { return obj.id(); }

        inline static type create(const wrapped_type& t) { return type(t); }

        template <typename T2>
        inline static sbk::core::child_ptr<T2> convert(const type& source, bool& ok)
        {
            sbk::core::child_ptr<T2> convertedLazyPtr(source.id());

            ok = source.hasId() == convertedLazyPtr.hasId();

            return convertedLazyPtr;
        }
    };

    template <typename T>
    struct wrapper_mapper<sbk::core::database_ptr<T>>
    {
        using wrapped_type = decltype(sbk::core::database_ptr<T>().id());
        using type         = sbk::core::database_ptr<T>;

        inline static wrapped_type get(const type& obj) { return obj.id(); }

        inline static type create(const wrapped_type& t) { return type(t); }

        template <typename T2>
        inline static sbk::core::database_ptr<T2> convert(const type& source, bool& ok)
        {
            sbk::core::database_ptr<T2> convertedLazyPtr(source.id());

            ok = source.hasId() == convertedLazyPtr.hasId();

            return convertedLazyPtr;
        }
    };

}  // namespace rttr
