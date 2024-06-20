#include "node.h"

#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::NodeBase)

DEFINE_REFLECTION(sbk::engine::Node)

void sbk::engine::Node::gatherParameters(GlobalParameterList& parameters)
{
    parameters.floatParameters.reserve(m_childNodes.size() + 1);
    parameters.intParameters.reserve(m_childNodes.size() + 1);

    gatherParametersFromThis(parameters);

    for (NodeBase* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (Node* const childNode = child->try_convert_object<Node>())
            {
                childNode->gatherParameters(parameters);
            }
        }
    }
}

void Node::addEffect(sc_dsp_type type)
{
    std::shared_ptr<effect_description> effect = owner()->create_database_object<effect_description>();
    effect->set_dsp_type(type);
    m_effectDescriptions.emplace_back(effect);
}

NodeBase::~NodeBase()
{
    for (auto& child : m_childNodes)
    {
        if (child.valid())
        {
            // SB::Core::Database::get()->remove(child.id());
        }
    }
}

void sbk::engine::NodeBase::setParentNode(const sbk::core::database_ptr<NodeBase>& parent) { m_parentNode = parent; }

void sbk::engine::NodeBase::setOutputBus(const sbk::core::database_ptr<NodeBase>& bus) { m_outputBus = bus; }

SB_NODE_STATUS sbk::engine::NodeBase::getNodeStatus() const noexcept
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

NodeBase* sbk::engine::NodeBase::parent() const { return m_parentNode.lookupRaw(); }

NodeBase* sbk::engine::NodeBase::outputBus() const { return m_outputBus.lookupRaw(); }

bool sbk::engine::NodeBase::canAddChild(const sbk::core::database_ptr<NodeBase>& child) const
{
    if (m_childNodes.contains(child) || child.id() == get_database_id())
    {
        return false;
    }
    return true;
}

void sbk::engine::NodeBase::addChild(const sbk::core::database_ptr<NodeBase>& child)
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

void sbk::engine::NodeBase::removeChild(const sbk::core::database_ptr<NodeBase>& child)
{
    if (child)
    {
        child->setParentNode(nullptr);
    }

    m_childNodes.erase(child);
}

std::vector<NodeBase*> sbk::engine::NodeBase::getChildren() const
{
    std::vector<NodeBase*> children;
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

std::size_t sbk::engine::NodeBase::getChildCount() const { return m_childNodes.size(); }

bool sbk::engine::NodeBase::hasChild(const sbk::core::database_ptr<NodeBase>& test) const
{
    return m_childNodes.contains(test);
}

void sbk::engine::NodeBase::gatherAllDescendants(std::vector<NodeBase*>& descendants) const
{
    for (auto& child : getChildren())
    {
        descendants.push_back(child);

        child->gatherAllDescendants(descendants);
    }
}

void sbk::engine::NodeBase::gatherAllParents(std::vector<NodeBase*>& parents) const
{
    if (NodeBase* const nodeParent = parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gatherAllParents(parents);
    }
}
