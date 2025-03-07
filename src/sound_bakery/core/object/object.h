#pragma once

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/core/memory.h"
#include "sound_bakery/util/leak_detector.h"
#include <boost/core/noncopyable.hpp>
#include <boost/serialization/nvp.hpp>

namespace sbk::core
{
    enum object_flags
    {
        object_flag_none    = 0x00000000, 
        object_flag_loading = 0x00000001  //< Serializing is loading property data
    };

    /**
     * @brief Base object that all sound Bakery objects should inherit
     * from.
     *
     * Objects can own other objects.
     */
    class SB_CLASS object : public object_owner, public std::enable_shared_from_this<object>, public boost::noncopyable
    {
        REGISTER_REFLECTION(object)
        LEAK_DETECTOR(object)

    public:
        object() = default;
        virtual ~object();

        template <typename T>
        [[nodiscard]] auto casted_shared_from_this() -> std::shared_ptr<T>;
        
        /**
         * @brief Gets the most derived type of this object and upcasts it to T
         */
        template <typename T>
        [[nodiscard]] auto try_convert_object() noexcept -> T*;

        /**
         * @brief Const version of try_convert_object.
         */
        template <typename T>
        [[nodiscard]] auto try_convert_object() const noexcept -> const T*;

        auto destroy() -> void;

        [[nodiscard]] auto get_object_type() const -> rttr::type;
        [[nodiscard]] auto get_owner() const -> object_owner*;
        [[nodiscard]] auto get_owner_object() const -> object*;
        [[nodiscard]] auto get_on_destroy() -> MulticastDelegate<object*>&;

        [[nodiscard]] auto get_flags() const -> object_flags;
        [[nodiscard]] auto set_flags(object_flags flagsToSet) -> void;
        [[nodiscard]] auto clear_flags(object_flags flagsToClear) -> void;
        [[nodiscard]] auto has_flag(object_flags flagsToCheck) -> bool;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int fileVersion)
        {
            const rttr::type type = get_object_type();
            BOOST_ASSERT(type.is_valid());

            for (rttr::property property : type.get_properties())
            {
                BOOST_VERIFY(property.is_valid());
                std::string propertyName = property.get_name().data();
                std::replace(propertyName.begin(), propertyName.end(), ' ', '_');

                if (typename archive_class::is_saving())
                {
                    rttr::variant propertyVariant = property.get_value(rttr::instance(this));
                    archive & boost::serialization::make_nvp(propertyName.c_str(), propertyVariant);
                }
                else if (typename archive_class::is_loading())
                {
                    rttr::variant loadedVariant = property.get_value(rttr::instance(this));
                    BOOST_VERIFY(loadedVariant.is_valid());
                    BOOST_VERIFY(loadedVariant.get_type().is_valid());
                    archive & boost::serialization::make_nvp(propertyName.c_str(), loadedVariant);

                    if (property.get_type() == rttr::type::get<std::string_view>() &&
                        loadedVariant.get_type() == rttr::type::get<std::string>())
                    {
                        std::string loadedString = loadedVariant.convert<std::string>();
                        std::string_view loadedStringView = loadedString;

                        if (has_flag(object_flag_loading))
                        {
                            property.set_value(rttr::instance(this), loadedStringView);
                        }
                    }
                    else
                    {
                        if (has_flag(object_flag_loading))
                        {
                            loadedVariant.convert(property.get_type());
                            BOOST_ASSERT(loadedVariant.get_type() == property.get_type());
                            property.set_value(rttr::instance(this), loadedVariant);
                        }
                    }
                }
            }
        }

        static void* operator new(std::size_t size)
        {
            BOOST_ASSERT_MSG(false, "Objects should not be created with default new");
            return std::malloc(size);
        }

        static void* operator new(std::size_t size, SB_OBJECT_CATEGORY category)
        {
            return sbk::memory::malloc(size, category);
        }

        static void operator delete(void* pointer)
        {
            BOOST_ASSERT_MSG(false, "Objects should not be created or destroyed with default new");
            std::free(pointer);
        }

        static void operator delete(void* pointer, SB_OBJECT_CATEGORY category)
        { 
            return sbk::memory::free(pointer, category);
        }

    private:
        friend class object_owner;

        auto set_owner(object_owner* newOwner) -> void;
        auto cache_type() -> void;

        object_owner* m_owner = nullptr;
        object_flags m_flags  = object_flag_none;

        /**
         * @brief Cache of this object's type so it can be grabbed during
         * destruction
         */
        mutable std::optional<rttr::type> m_type = std::nullopt;

        MulticastDelegate<object*> m_onDestroyEvent;
    };

#include "object.inl"
}  // namespace sbk::core