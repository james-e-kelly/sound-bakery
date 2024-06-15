#include "container.h"

#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/system.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::Container)

void Container::onLoaded()
{
    //if (!m_parentNode.hasId())
    //{
    //    const sbk::core::DatabasePtr<Bus>& masterBus = sbk::engine::system::get()->getMasterBus();
    //    if (!m_outputBus.hasId())
    //    {
    //        setOutputBus(sbk::core::DatabasePtr<NodeBase>(masterBus));
    //    }
    //    // Ensure we're pointing to the correct master
    //    else if (m_outputBus.id() != masterBus.id())
    //    {
    //        setOutputBus(sbk::core::DatabasePtr<NodeBase>(masterBus));
    //    }
    //}
}