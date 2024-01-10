#include "node.h"

#include "sound_bakery/system.h"
#include "sound_bakery/core/object/object_global.h"

using namespace SB::Engine;

void Node::addEffect(SC_DSP_TYPE type)
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
			//SB::Core::Database::get()->remove(child.id());
		}
	}
}
