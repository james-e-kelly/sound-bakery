#pragma once

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/pch.h"
#include "sound_bakery/util/macros.h"

namespace sbk::engine
{
    class system;
}

namespace sbk::core
{
    /**
     * @brief Base object that all Sound Bakery objects should inherit
     * from.
     *
     * Objects can own other objects.
     */
    class SB_CLASS object : public object_owner, public std::enable_shared_from_this<object>
    {
        REGISTER_REFLECTION(object)
        NOT_COPYABLE(object)

    public:
        object() = default;
        virtual ~object();

        [[nodiscard]] object_owner* owner() const { return m_owner; }

        /**
         * @brief Gets the most derived type of this object and upcasts it to T
         * @tparam T
         * @return
         */
        template <typename T>
        T* try_convert_object() noexcept
        {
            if (getType().is_derived_from(T::type()) || getType() == T::type())
            {
                return sbk::reflection::cast<T*, object*>(this);
            }
            return nullptr;
        }

        template <typename T>
        const T* try_convert_object() const noexcept
        {
            if (getType().is_derived_from(T::type()) || getType() == T::type())
            {
                return sbk::reflection::cast<const T*, const object*>(this);
            }
            return nullptr;
        }

        rttr::type getType() const
        {
            if (this == nullptr)
            {
                return rttr::type::get<void>();
            }

            if (!m_type.has_value())
            {
                m_type = get_type();
            }

            assert(m_type.has_value());
            assert(m_type.value().is_valid());

            return m_type.value();
        }

        [[nodiscard]] MulticastDelegate<object*>& get_on_destroy() { return m_onDestroyEvent; }

    private:
        friend class object_owner;

        void set_owner(object_owner* newOwner)
        {
            assert(m_owner == nullptr);
            m_owner = newOwner;
        }

        object_owner* m_owner = nullptr;

        /**
         * @brief Cache of this object's type so it can be grabbed during
         * destruction
         */
        mutable std::optional<rttr::type> m_type = std::nullopt;

        MulticastDelegate<object*> m_onDestroyEvent;
    };
}  // namespace sbk::core