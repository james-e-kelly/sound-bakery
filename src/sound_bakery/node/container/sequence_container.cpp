#include "sequence_container.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::sequence_container)

void sequence_container::gather_children_for_play(gather_children_context& context) const
{
    if (!m_sequence.empty() && context.numTimesPlayed > 0)
    {
        const unsigned int wrappedNumTimesPlayed = context.numTimesPlayed - 1;

        if (wrappedNumTimesPlayed < m_sequence.size())
        {
            context.sounds.push_back(m_sequence[wrappedNumTimesPlayed].lookupRaw());
        }
    }
}