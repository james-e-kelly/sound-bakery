#include "sound_bakery/core/object/object.h"

#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/system.h"

DEFINE_REFLECTION(sbk::core::object)

sbk::core::object::~object() { m_onDestroyEvent.Broadcast(this); }

auto sbk::core::object::get_owner() const -> object_owner* { return m_owner; }

auto sbk::core::object::get_owner_object() const -> object* { return static_cast<sbk::core::object*>(get_owner()); }

auto sbk::core::object::get_on_destroy() -> MulticastDelegate<object*>& { return m_onDestroyEvent; }

auto sbk::core::object::set_owner(object_owner* newOwner) -> void
{
    BOOST_ASSERT(m_owner == nullptr);
    m_owner = newOwner;
}

auto sbk::core::object::cache_type() -> void
{
    if (!m_type.has_value())
    {
        m_type = get_type();
    }
}

auto sbk::core::object::get_object_type() const -> rttr::type
{
    if (this == nullptr)
    {
        return rttr::type::get<void>();
    }

    if (!m_type.has_value())
    {
        m_type = get_type();
    }

    BOOST_ASSERT(m_type.has_value());
    BOOST_ASSERT(m_type.value().is_valid());

    return m_type.value();
}

void sbk::core::object::destroy()
{
    if (m_owner)
    {
        m_owner->remove_object(shared_from_this());
    }
}

auto sbk::core::object::get_flags() const -> object_flags
{ 
    return m_flags; 
}

auto sbk::core::object::set_flags(object_flags flagsToSet) -> void 
{
    m_flags = static_cast<object_flags>(m_flags | flagsToSet);
}

auto sbk::core::object::clear_flags(object_flags flagsToClear) -> void 
{ 
    m_flags = static_cast<object_flags>(m_flags & ~flagsToClear);
}

auto sbk::core::object::has_flag(object_flags flagsToCheck) -> bool
{
    return (m_flags & flagsToCheck) == flagsToCheck;
}