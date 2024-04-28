#include "node.h"

#include "sound_bakery/core/object/object_global.h"
#include "sound_bakery/system.h"

using namespace SB::Engine;

DEFINE_REFLECTION(SB::Engine::NodeBase)

DEFINE_REFLECTION(SB::Engine::Node)

void SB::Engine::Node::gatherParameters(GlobalParameterList& parameters)
{
    parameters.floatParameters.reserve(m_childNodes.size() + 1);
    parameters.intParameters.reserve(m_childNodes.size() + 1);

    gatherParametersFromThis(parameters);

    for (NodeBase* const child : getChildren())
    {
        if (child != nullptr)
        {
            if (Node* const childNode = child->tryConvertObject<Node>())
            {
                childNode->gatherParameters(parameters);
            }
        }
    }
}

void Node::addEffect(sc_dsp_type type)
{
    EffectDescription* effect = newDatabaseObject<EffectDescription>();
    effect->setDSPType(type);
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

void SB::Engine::NodeBase::setParentNode(const SB::Core::DatabasePtr<NodeBase>& parent) { m_parentNode = parent; }

void SB::Engine::NodeBase::setOutputBus(const SB::Core::DatabasePtr<NodeBase>& bus) { m_outputBus = bus; }

SB_NODE_STATUS SB::Engine::NodeBase::getNodeStatus() const noexcept
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

NodeBase* SB::Engine::NodeBase::parent() const { return m_parentNode.lookupRaw(); }

NodeBase* SB::Engine::NodeBase::outputBus() const { return m_outputBus.lookupRaw(); }

bool SB::Engine::NodeBase::canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const
{
    if (m_childNodes.contains(child) || child.id() == getDatabaseID())
    {
        return false;
    }
    return true;
}

void SB::Engine::NodeBase::addChild(const SB::Core::DatabasePtr<NodeBase>& child)
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

void SB::Engine::NodeBase::removeChild(const SB::Core::DatabasePtr<NodeBase>& child)
{
    if (child)
    {
        child->setParentNode(nullptr);
    }

    m_childNodes.erase(child);
}

std::vector<NodeBase*> SB::Engine::NodeBase::getChildren() const
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

std::size_t SB::Engine::NodeBase::getChildCount() const { return m_childNodes.size(); }

bool SB::Engine::NodeBase::hasChild(const SB::Core::DatabasePtr<NodeBase>& test) const
{
    return m_childNodes.contains(test);
}

void SB::Engine::NodeBase::gatherAllDescendants(std::vector<NodeBase*>& descendants) const
{
    for (auto& child : getChildren())
    {
        descendants.push_back(child);

        child->gatherAllDescendants(descendants);
    }
}

void SB::Engine::NodeBase::gatherAllParents(std::vector<NodeBase*>& parents) const
{
    if (NodeBase* const nodeParent = parent())
    {
        parents.push_back(nodeParent);

        nodeParent->gatherAllParents(parents);
    }
}
