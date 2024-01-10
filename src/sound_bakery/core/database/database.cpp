#include "database.h"

static std::random_device s_RandomDevice;
static std::mt19937_64 s_Engine(s_RandomDevice());
static std::uniform_int_distribution<SB_ID> s_UniformDistribution;

SB_ID SB::Core::Database::createNewID()
{
    return s_UniformDistribution(s_Engine);
}
