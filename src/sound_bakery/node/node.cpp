#include "node.h"

#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::node_base)

DEFINE_REFLECTION(sbk::engine::node)

void sbk::engine::node::gatherParameters(GlobalParameterList& parameters)
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

void sbk::engine::node_base::setParentNode(const sbk::core::database_ptr<node_base>& parent) { m_parentNode = parent; }

void sbk::engine::node_base::setOutputBus(const sbk::core::database_ptr<node_base>& bus) { m_outputBus = bus; }

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

node_base* sbk::engine::node_base::parent() const { return m_parentNode.lookupRaw(); }

node_base* sbk::engine::node_base::outputBus() const { return m_outputBus.lookupRaw(); }

bool sbk::engine::node_base::canAddChild(const sbk::core::database_ptr<node_base>& child) const
{
    if (m_childNodes.contains(child) || child.id() == get_database_id())
    {
        return false;
    }
    return true;
}

void sbk::engine::node_base::addChild(const sbk::core::database_ptr<node_base>& child)
{
    if (canAddChild(child))
    {
        if (child.lookup() && child->parent())
        {
            child->parent()->removeChild(child);
        }

        m_childNodes.insert(child);

        if (child.lookup())
        {
            child->setParentNode(this);
        }
    }
}

void sbk::engine::node_base::removeChild(const sbk::core::database_ptr<node_base>& child)
{
    if (child)
    {
        child->setParentNode(nullptr);
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
    if (node_base* const nodeParent = parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gatherAllParents(parents);
    }
}
