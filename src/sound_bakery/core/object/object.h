#pragma once

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/util/leak_detector.h"
#include "sound_bakery/util/macros.h"

namespace sbk::engine
{
    class system;
}

namespace sbk::core
{
    /**
     * @brief Base object that all sound Bakery objects should inherit
     * from.
     *
     * Objects can own other objects.
     */
    class SB_CLASS object : public object_owner, public std::enable_shared_from_this<object>
    {
        REGISTER_REFLECTION(object)
        NOT_COPYABLE(object)
        LEAK_DETECTOR(object)

    public:
        object() = default;
        virtual ~object();

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
        [[nodiscard]] auto get_on_destroy() -> MulticastDelegate<object*>&;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int fileVersion)
        {
            const rttr::type type = get_object_type();
            BOOST_ASSERT(type.is_valid());

            float test = 0.7f;

            for (rttr::property property : type.get_properties())
            {
                if (archive_class::is_saving())
                {
                    archive & test;
                    //archive & property.get_value(rttr::instance(this));
                }
                else if (archive_class::is_loading())
                {
                    archive & test;
                    //rttr::variant loadedVariant;
                    //archive & loadedVariant;
                    //property.set_value(rttr::instance(this), loadedVariant);
                }
            }
        }

    private:
        friend class object_owner;

        auto set_owner(object_owner* newOwner) -> void;
        auto cache_type() -> void;

        object_owner* m_owner = nullptr;

        /**
         * @brief Cache of this object's type so it can be grabbed during
         * destruction
         */
        mutable std::optional<rttr::type> m_type = std::nullopt;

        MulticastDelegate<object*> m_onDestroyEvent;
    };

#include "object.inl"
}  // namespace sbk::core