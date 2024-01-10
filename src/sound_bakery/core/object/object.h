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
    class ObjectUtilities
    {
    public:
        SB::Engine::System* getSystem() const;
        SC_SYSTEM* getChef() const;
        ma_engine* getMini() const;
        rttr::type getType() const;

        RTTR_ENABLE()
    };

	/**
	 * @brief Simple base Object that all Sound Bakery objects should inherit from
	*/
	class Object : public ObjectUtilities
    {
    public:
        Object() = default;
        virtual ~Object();

        NOT_COPYABLE(Object)

        /**
         * @brief Gets the most derived type of this object and upcasts it to T
         * @tparam T 
         * @return 
        */
        template<typename T>
        T* tryConvertObject() noexcept
        {
            if (getType().is_derived_from<T>() || getType() == rttr::type::get<T>())
            {
                return rttr::rttr_cast<T*, Object*>(this);
            }
            return nullptr;
        }

        rttr::type getType() const
        {
            if (m_type == rttr::type::get<void>())
            {
                m_type = get_type();
            }

            return m_type;
        }

    private:
        /**
         * @brief Cache of this object's type so it can be grabbed during destruction
        */
        mutable rttr::type m_type = rttr::type::get<void>();

        RTTR_ENABLE(ObjectUtilities)
	};
}