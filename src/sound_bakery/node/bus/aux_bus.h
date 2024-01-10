#pragma once

#include "bus.h"

namespace SB::Engine
{
	class AuxBus : public Bus
	{

		RTTR_ENABLE(Bus)
		RTTR_REGISTRATION_FRIEND
	};
}