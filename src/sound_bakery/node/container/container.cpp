#include "container.h"

#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/system.h"

using namespace SB::Engine;

void Container::onLoaded()
{
    if (!m_parentNode.hasId())
    {
        const SB::Core::DatabasePtr<Bus>& masterBus =
            SB::Engine::System::get()->getMasterBus();
        if (!m_outputBus.hasId())
        {
            setOutputBus(SB::Core::DatabasePtr<NodeBase>(masterBus));
        }
        // Ensure we're pointing to the correct master
        else if (m_outputBus.id() != masterBus.id())
        {
            setOutputBus(SB::Core::DatabasePtr<NodeBase>(masterBus));
        }
    }
}