#include "weighted_list.h"

#include "rng.h"

namespace detail
{
unsigned int gen_rand_i()
{
    return rng_bits();
}
} // namespace detail
