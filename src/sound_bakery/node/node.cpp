#include "node.h"

#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::node_base)

DEFINE_REFLECTION(sbk::engine::node)

auto sbk::engine::node::gather_parameters(global_parameter_list& parameters) -> void
{
    parameters.floatParameters.reserve(m_childNodes.size() + 1);
    parameters.intParameters.reserve(m_childNodes.size() + 1);

    gather_parameters_from_this(parameters);

    for (auto& child : get_children())
    {
        if (child)
        {
            if (auto childNode = std::static_pointer_cast<node>(child))
            {
                childNode->gather_parameters(parameters);
            }
        }
    }
}

auto node::add_effect(sc_dsp_type type) -> sbk::result<void>
{
    SBK_CHECK(get_owner() != nullptr, SBK_ERR_NULL);
    SBK_TRY(auto effect, get_owner()->create_database_object<effect_description>());
    effect->set_dsp_type(type);
    m_effectDescriptions.emplace_back(effect);
    return sbk::ok();
}

auto node::add_effect_clap(const clap_plugin_factory_t* clapFactory) -> sbk::result<void>
{
    SBK_CHECK(get_owner() != nullptr, SBK_ERR_NULL);
    SBK_TRY(auto effect, get_owner()->create_database_object<effect_description>());
    effect->set_dsp_clap(clapFactory);
    m_effectDescriptions.emplace_back(effect);
    return sbk::ok();
}

node_base::~node_base()
{
    for (auto& child : m_childNodes)
    {
        if (child.valid())
        {
            // SB::Core::Database::get()->remove(child.id());
        }
    }
}

auto sbk::engine::node_base::set_parent_node(const sbk::core::database_ptr<node_base>& parent) -> void
{
    m_parentNode = parent;
}

auto sbk::engine::node_base::set_output_bus(const sbk::core::database_ptr<node_base>& bus) -> void { m_outputBus = bus; }

auto sbk::engine::node_base::get_node_status() const noexcept -> node_status
{
    node_status status = node_status::null;

    if (m_parentNode.has_id())
    {
        status = node_status::middle;
    }
    else if (m_outputBus.has_id())
    {
        status = node_status::top;
    }

    return status;
}

auto sbk::engine::node_base::get_parent() const -> std::shared_ptr<node_base> { return m_parentNode.shared(); }

auto sbk::engine::node_base::get_output_bus() const -> std::shared_ptr<node_base> { return m_outputBus.shared(); }

auto sbk::engine::node_base::can_add_children() const -> bool { return true; }

auto sbk::engine::node_base::can_add_child_type(const rttr::type& childType) const -> bool
{
    return childType.is_valid() && childType.is_derived_from<sbk::engine::node_base>();
}

auto sbk::engine::node_base::can_add_child(const sbk::core::database_ptr<node_base>& child) const -> bool
{
    if (auto childShared = child.shared())
    {
        const bool canAddChildren         = can_add_children();
        const bool canAddType             = can_add_child_type(childShared->get_type());
        const bool childIsNotAlreadyChild = !m_childNodes.contains(child);
        const bool childIsNotSelf         = child.id() != get_database_id();

        return canAddChildren && canAddType && childIsNotAlreadyChild && childIsNotSelf;
    }
    return false;
}

auto sbk::engine::node_base::can_add_parent() const -> bool { return true; }

auto sbk::engine::node_base::can_add_parent_type(const rttr::type& parentType) const -> bool
{
    return parentType.is_valid() && parentType.is_derived_from<sbk::engine::node_base>();
}

auto sbk::engine::node_base::add_child(const sbk::core::database_ptr<node_base>& child) -> void
{
    if (can_add_child(child))
    {
        if (auto childShared = child.shared())
        {
            if (auto parent = childShared->get_parent())
            {
                parent->remove_child(child);
            }

            childShared->set_parent_node(this);
        }

        m_childNodes.insert(child);
    }
}

auto sbk::engine::node_base::remove_child(const sbk::core::database_ptr<node_base>& child) -> void
{
    if (auto childShared = child.shared())
    {
        childShared->set_parent_node(nullptr);
    }

    m_childNodes.erase(child);
}

auto sbk::engine::node_base::get_children() const -> eastl::vector<std::shared_ptr<node_base>>
{
    eastl::vector<std::shared_ptr<node_base>> children;
    children.reserve(m_childNodes.size());

    for (const auto& child : m_childNodes)
    {
        if (auto childShared = child.shared())
        {
            children.push_back(childShared);
        }
    }

    return children;
}

auto sbk::engine::node_base::get_child_count() const -> std::size_t { return m_childNodes.size(); }

auto sbk::engine::node_base::has_child(const sbk::core::database_ptr<node_base>& test) const -> bool
{
    return m_childNodes.contains(test);
}

auto sbk::engine::node_base::gather_all_descendants(eastl::vector<std::shared_ptr<node_base>>& descendants) const -> void
{
    for (auto& child : get_children())
    {
        descendants.push_back(child);

        child->gather_all_descendants(descendants);
    }
}

auto sbk::engine::node_base::gather_all_parents(eastl::vector<std::shared_ptr<node_base>>& parents) const -> void
{
    if (auto nodeParent = get_parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gather_all_parents(parents);
    }
}

