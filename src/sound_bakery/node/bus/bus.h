#pragma once

#include "sound_bakery/node/node.h"

namespace sbk::engine
{
    class node_instance;

    class SB_CLASS bus : public node
    {
        REGISTER_REFLECTION(bus, node)
        LEAK_DETECTOR(bus)

    public:
        bus() : node(), m_masterBus(false) {}

        [[nodiscard]] auto can_add_child_type(const rttr::type& childType) const -> bool override;

        [[nodiscard]] auto can_add_parent() const -> bool override;
        [[nodiscard]] auto can_add_parent_type(const rttr::type& parentType) const -> bool override;

        auto set_master_bus(bool isMaster) -> void;

        [[nodiscard]] auto is_master_bus() const -> bool { return m_masterBus; }

        [[nodiscard]] auto lock_and_copy() -> std::shared_ptr<node_instance>;

    protected:
        std::weak_ptr<node_instance> m_busInstance;

    private:
        bool m_masterBus;
    };
}  // namespace sbk::engine