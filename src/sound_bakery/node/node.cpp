#include "node.h"

#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::node_base)

DEFINE_REFLECTION(sbk::engine::node)

void sbk::engine::node::gatherParameters(global_parameter_list& parameters)
{
    parameters.floatParameters.reserve(m_childNodes.size() + 1);
    parameters.intParameters.reserve(m_childNodes.size() + 1);

    gatherParametersFromThis(parameters);

    for (node_base* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (node* const childNode = child->try_convert_object<node>())
            {
                childNode->gatherParameters(parameters);
            }
        }
    }
}

void node::addEffect(sc_dsp_type type)
{
    std::shared_ptr<effect_description> effect = owner()->create_database_object<effect_description>();
    effect->set_dsp_type(type);
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

void sbk::engine::node_base::set_parent_node(const sbk::core::database_ptr<node_base>& parent)
{
    m_parentNode = parent;
}

void sbk::engine::node_base::set_output_bus(const sbk::core::database_ptr<node_base>& bus) { m_outputBus = bus; }

SB_NODE_STATUS sbk::engine::node_base::getNodeStatus() const noexcept
{
    SB_NODE_STATUS status = SB_NODE_NULL;

    if (m_parentNode.hasId())
    {
        status = SB_NODE_MIDDLE;
    }
    else if (m_outputBus.hasId())
    {
        status = SB_NODE_TOP;
    }

    return status;
}

node_base* sbk::engine::node_base::get_parent() const { return m_parentNode.lookupRaw(); }

node_base* sbk::engine::node_base::get_output_bus() const { return m_outputBus.lookupRaw(); }

bool sbk::engine::node_base::can_add_children() const { return true; }

bool sbk::engine::node_base::can_add_child_type(const rttr::type& childType) const
{
    return childType.is_valid() && childType.is_derived_from<sbk::engine::node_base>();
}

bool sbk::engine::node_base::can_add_child(const sbk::core::database_ptr<node_base>& child) const
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

bool sbk::engine::node_base::can_add_parent() const { return true; }

bool sbk::engine::node_base::can_add_parent_type(const rttr::type& parentType) const
{
    return parentType.is_valid() && parentType.is_derived_from<sbk::engine::node_base>();
}

void sbk::engine::node_base::addChild(const sbk::core::database_ptr<node_base>& child)
{
    if (can_add_child(child))
    {
        if (child.lookup() && child->get_parent())
        {
            child->get_parent()->removeChild(child);
        }

        m_childNodes.insert(child);

        if (child.lookup())
        {
            child->set_parent_node(this);
        }
    }
}

void sbk::engine::node_base::removeChild(const sbk::core::database_ptr<node_base>& child)
{
    if (child)
    {
        child->set_parent_node(nullptr);
    }

    m_childNodes.erase(child);
}

std::vector<node_base*> sbk::engine::node_base::getChildren() const
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

std::size_t sbk::engine::node_base::getChildCount() const { return m_childNodes.size(); }

bool sbk::engine::node_base::hasChild(const sbk::core::database_ptr<node_base>& test) const
{
    return m_childNodes.contains(test);
}

void sbk::engine::node_base::gatherAllDescendants(std::vector<node_base*>& descendants) const
{
    for (auto& child : getChildren())
    {
        descendants.push_back(child);

        child->gatherAllDescendants(descendants);
    }
}

void sbk::engine::node_base::gatherAllParents(std::vector<node_base*>& parents) const
{
    if (node_base* const nodeParent = get_parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gatherAllParents(parents);
    }
}
