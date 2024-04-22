#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/util/macros.h"

namespace SB::Engine
{
    class System;
}

namespace SB::Core
{
    /**
     * @brief Provides basic helper functions. Not meant to be used directly
     */
    class SB_CLASS ObjectUtilities
    {
    public:
        SB::Engine::System* getSystem() const;
        SC_SYSTEM* getChef() const;
        ma_engine* getMini() const;
        rttr::type getType() const;
        
        std::string m_debugName;

        REGISTER_REFLECTION(ObjectUtilities)
    };

    /**
     * @brief Simple base Object that all Sound Bakery objects should inherit
     * from
     */
    class SB_CLASS Object : public ObjectUtilities
    {
        REGISTER_REFLECTION(Object, ObjectUtilities)

    public:
        Object() = default;
        virtual ~Object();

        NOT_COPYABLE(Object)

        /**
         * @brief Gets the most derived type of this object and upcasts it to T
         * @tparam T
         * @return
         */
        template <typename T>
        T* tryConvertObject() noexcept
        {
            if (getType().is_derived_from(T::type()) || getType() == T::type())
            {
                return SB::Reflection::cast<T*, Object*>(this);
            }
            return nullptr;
        }

        template <typename T>
        const T* tryConvertObject() const noexcept
        {
            if (getType().is_derived_from(T::type()) || getType() == T::type())
            {
                return SB::Reflection::cast<const T*, const Object*>(this);
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

    private:
        /**
         * @brief Cache of this object's type so it can be grabbed during
         * destruction
         */
        mutable std::optional<rttr::type> m_type = std::nullopt;
    };
}  // namespace SB::Core