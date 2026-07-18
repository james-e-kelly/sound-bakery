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

    for (node_base* const child : get_children())
    {
        if (child != nullptr)
        {
            if (node* const childNode = child->try_convert_object<node>())
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
}

auto node::add_effect_clap(clap_plugin_factory_t* clapFactory) -> sbk::result<void>
{
    SBK_CHECK(get_owner() != nullptr, SBK_ERR_NULL);
    SBK_TRY(auto effect, get_owner()->create_database_object<effect_description>());
    effect->set_dsp_clap(clapFactory);
    m_effectDescriptions.emplace_back(effect);
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
    if (m_onParentUpdateNameDelegate.IsValid())
    {
        if (std::shared_ptr<node_base> currentParent = m_parentNode.shared())
        {
            currentParent->get_on_update_name().Remove(m_onParentUpdateNameDelegate);
        }
    }

    m_parentNode = parent;

    if (std::shared_ptr<node_base> newParent = m_parentNode.shared())
    {
        m_onParentUpdateNameDelegate = newParent->get_on_update_database_name().AddRaw(this, &node_base::on_parent_update_database_name);
    }
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

auto sbk::engine::node_base::get_parent() const -> node_base* { return m_parentNode.lookup_raw(); }

auto sbk::engine::node_base::get_output_bus() const -> node_base* { return m_outputBus.lookup_raw(); }

auto sbk::engine::node_base::can_add_children() const -> bool { return true; }

auto sbk::engine::node_base::can_add_child_type(const rttr::type& childType) const -> bool
{
    return childType.is_valid() && childType.is_derived_from<sbk::engine::node_base>();
}

auto sbk::engine::node_base::can_add_child(const sbk::core::database_ptr<node_base>& child) const -> bool
{
    if (child.lookup())
    {
        const bool canAddChildren         = can_add_children();
        const bool canAddType             = can_add_child_type(child->get_type());
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
        if (child.lookup() && child->get_parent())
        {
            child->get_parent()->remove_child(child);
        }

        m_childNodes.insert(child);

        if (child.lookup())
        {
            child->set_parent_node(this);
        }
    }
}

auto sbk::engine::node_base::remove_child(const sbk::core::database_ptr<node_base>& child) -> void
{
    if (child)
    {
        child->set_parent_node(nullptr);
    }

    m_childNodes.erase(child);
}

auto sbk::engine::node_base::get_children() const -> std::vector<node_base*>
{
    std::vector<node_base*> children;
    children.reserve(m_childNodes.size());

    for (auto& child : m_childNodes)
    {
        if (child.lookup())
        {
            children.push_back(child.raw());
        }
    }

    return children;
}

auto sbk::engine::node_base::get_child_count() const -> std::size_t { return m_childNodes.size(); }

auto sbk::engine::node_base::has_child(const sbk::core::database_ptr<node_base>& test) const -> bool
{
    return m_childNodes.contains(test);
}

auto sbk::engine::node_base::gather_all_descendants(std::vector<node_base*>& descendants) const -> void
{
    for (auto& child : get_children())
    {
        descendants.push_back(child);

        child->gather_all_descendants(descendants);
    }
}

auto sbk::engine::node_base::gather_all_parents(std::vector<node_base*>& parents) const -> void
{
    if (node_base* const nodeParent = get_parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gather_all_parents(parents);
    }
}

auto sbk::engine::node_base::on_parent_update_database_name(const sbk::core::database_name& oldName,
                                                            const sbk::core::database_name& newName) -> void
{
    const sbk::core::database_name thisOldDatabaseName = oldName / get_object_name();
    const sbk::core::database_name thisNewDatabaseName = newName / get_object_name();
    get_on_update_database_name().Broadcast(thisOldDatabaseName, thisNewDatabaseName);
}