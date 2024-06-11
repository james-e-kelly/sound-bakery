#include "database.h"

sb_id SB::Core::database::create_new_id()
{
    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<sb_id> s_UniformDistribution;

    return s_UniformDistribution(s_Engine);
}
