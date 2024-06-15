#include "sequence_container.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::SequenceContainer)

void SequenceContainer::gatherChildrenForPlay(GatherChildrenContext& context) const
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