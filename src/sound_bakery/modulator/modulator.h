#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/property.h"

namespace sbk::engine
{
    /**
     * @brief Defines an object that can modulate a @ref parameter;
     */
	class SB_CLASS modulator : public sbk::core::database_object
	{
    public:

    private:
        sbk::core::database_ptr<sbk::core::float_property> m_property;
	};
}