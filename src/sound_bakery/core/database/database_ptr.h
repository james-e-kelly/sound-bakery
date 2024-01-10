#pragma once

#include "sound_bakery/pch.h"

namespace SB::Core
{
    class DatabaseObject;

    std::weak_ptr<DatabaseObject> findObject(SB_ID id);

    /** Lazy Pointer
     * Lazy pointers store an Indentifier to an object and use it to find the object later
     * Lazy pointers can be serialized so references can persist through multiple shutdowns
     * Passing only an indentifier will create a pointer that references no object but will try to find
     * the object when accessing the object
    */ 
    template<typename TObject>
    class DatabasePtr final
    {
    public:
        using TThisType = DatabasePtr<TObject>;
        using TIdentifierType = SB_ID;
        using TObjectPtr = TObject*;
        using TObjectRef = TObject&;
        using TObjectWeak = std::weak_ptr<DatabaseObject>;
        using TObjectShared = std::shared_ptr<TObject>;
        using TPtrType = std::weak_ptr<DatabaseObject>;

        static_assert(!std::is_pointer<TObject>::value);

    public:
        /**
         * @brief Creates an empty and null LazyPtr
        */
        DatabasePtr()
            : m_objectID(), m_objectPtr(), m_null(true){}

        DatabasePtr(const TThisType& other) = default;
        DatabasePtr(TThisType&& other) = default;

        /**
         * @brief Creates a LazyPtr that can lookup its object pointer after construction
         * @param id ID of the object to reference
        */
        DatabasePtr(SB_ID id)
            : m_objectID(id), m_objectPtr(), m_null(true){}
          
        /**
         * @brief Create a valid LazyPtr
         * @param object 
        */
        DatabasePtr(TObjectPtr object)
            : m_objectID(static_cast<TIdentifierType>(*object)), m_objectPtr(findObject(id())), m_null(false)
        {}
        
        /**
         * @brief Create an empty and null LazyPtr
         * @param nullptr_t 
        */
        DatabasePtr(std::nullptr_t)
            : m_objectID(), m_objectPtr(), m_null(true)
        {}

        ~DatabasePtr() = default;

    public:
        /**
         * @brief Get ID of the referenced object
         * @return 
        */
        SB_ID id() const noexcept { return m_objectID; }

        TObjectShared shared() const noexcept 
        {
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

        /**
         * @brief Returns true if we hold a valid ID and can search for an object at runtime
         * @return 
        */
        bool hasId() const { return m_objectID != TIdentifierType(); }

        /**
         * @brief Returns true if the object pointer is not set
         * @return 
        */
        bool null() const { return m_null || m_objectPtr.expired(); }

        /**
         * @brief Returns true if we hold an ID but haven't found the live object to point to yet
         * @return 
        */
        bool pending() const { return hasId() && null(); }

        /**
         * @brief Returns true if we previously referenced an object that has been destroyed
         * @return 
        */
        bool stale() const { return !m_null && !m_objectPtr; }

        /**
         * @brief Returns true if we hold an ID and a valid pointer to the object
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
                m_null = m_objectPtr.expired();
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
        /**
         * @brief Assign this LazyPtr to a new object, potentially destroying the current object if we're acting as a UniquePtr
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
                m_objectID = other.id();
                m_objectPtr = other.weak();
                m_null = other.null();
            }

            return *this;
        }

        TThisType& operator=(const TThisType&& other)
        {
            if (id() != other.id())
            {
                m_objectID = other.id();
                m_objectPtr = other.weak();
                m_null = other.null();
            }

            return *this;
        }

        /**
         * @brief Returns true if this LazyPtr references a valid object
        */
        operator bool() const { return valid(); }

        /**
         * @brief Access the raw object
         * @return raw object
        */
        TObjectPtr operator->() const { return raw(); }

    private:
        SB_ID m_objectID;
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
    template<typename T1, typename T2>
    bool operator==(const DatabasePtr<T1>& lhs, const DatabasePtr<T2>& rhs)
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
    template<typename T>
    bool operator==(const DatabasePtr<T>& lhs, const T* rhs)
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
    template<typename T1, typename T2>
    bool operator<(const DatabasePtr<T1>& lhs, const DatabasePtr<T2>& rhs)
    {
        return lhs.id() < rhs.id();
    }
}

namespace std
{
    template <typename T>
    struct hash<SB::Core::DatabasePtr<T>>
    {
        size_t operator()(const SB::Core::DatabasePtr<T>& k) const
        {
            return hash<SB_ID>{}(k.id());
        }
    };
}

#include <rttr/wrapper_mapper.h>

namespace rttr
{
    template <typename T>
    struct wrapper_mapper<SB::Core::DatabasePtr<T>>
    {
        using wrapped_type = decltype(SB::Core::DatabasePtr<T>().id());
        using type = SB::Core::DatabasePtr<T>;

        inline static wrapped_type get(const type& obj)
        {
            return obj.id();
        }

        inline static type create(const wrapped_type& t)
        {
            return type(t);
        }

        template<typename T2>
        inline static SB::Core::DatabasePtr<T2> tryConvert(const type& source, bool& ok)
        {
            SB::Core::DatabasePtr<T2> convertedLazyPtr(source.id());

            ok = convertedLazyPtr.hasId();

            return convertedLazyPtr;
        }
    };
}