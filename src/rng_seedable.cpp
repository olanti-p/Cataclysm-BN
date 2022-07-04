#include "rng_seedable.h"

#include <random>
#include <sstream>

RNG::RNG( u64 seed )
{
    engine.set_state( seed );
}

std::string RNG::write_state() const
{
    T state = engine.get_state();

    std::stringstream ss;
    ss << state;
    return ss.str();
}

void RNG::load_from_state( const std::string &s )
{
    T state = 0;

    std::stringstream ss( s );
    ss >> state;

    engine.set_state( state );
}

u64 generate_seed_from_system()
{
    std::random_device dev;
    std::uniform_int_distribution<u64> dist;
    return dist( dev );
}
