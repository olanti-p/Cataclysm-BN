#pragma once
#ifndef CATA_SRC_RNG_SEEDABLE_H
#define CATA_SRC_RNG_SEEDABLE_H

#include <cstdint>
#include <limits>
#include <string>

#include "integer.h"

/**
 * @brief Xorshift64* PRNG generator
 *
 * Satisfies UniformRandomBitGenerator C++ requirement
 */
class Xorshift64_star
{
    public:
        using T = u64;

        using result_type = T;

        constexpr static T min() {
            // Cannot be zero
            return 1;
        }

        constexpr static T max() {
            return std::numeric_limits<T>::max();
        }

        constexpr inline u64 operator()() {
            T new_state = state;
            new_state ^= new_state >> 12;
            new_state ^= new_state << 25;
            new_state ^= new_state >> 27;
            state = new_state;
            return new_state * 2685821657736338717ULL;
        }

        constexpr inline T get_state() const {
            return state;
        }

        constexpr inline void set_state( T new_state ) {
            state = new_state;
        }

    private:
        T state = 0;
};

/**
 * @brief Universal deterministic cross-platform PRNG for game purposes.
 */
class RNG
{
    private:
        Xorshift64_star engine;
        using T = Xorshift64_star::T;

    public:
        RNG() = default;
        explicit RNG( u64 seed );
        ~RNG() = default;

        /**
         * Write state to string
         */
        std::string write_state() const;

        /**
         * Load state from string
         */
        void load_from_state( const std::string &s );

        /**
         * Generate `true` or `false` with equal chances
         */
        constexpr bool gen_bool() {
            return ( engine() & 0x1 ) == 0;
        }

        /**
         * Generate `true` with chance 1 in `chance`
         */
        constexpr bool one_in( u64 chance ) {
            u64 bound = std::numeric_limits<u64>::max() / chance;
            return engine() <= bound;
        }

        /**
         * Generate `true` with chance X in Y
         */
        constexpr bool x_in_y( double x, double y ) {
            return gen_double( 0.0, 1.0 ) <= x / y;
        }

        /**
         * Generate 64 random bits
         */
        constexpr u64 gen_bits() {
            return engine();
        }

        /**
         * Generate double in bounds [0.0, 1.0]
         */
        constexpr double gen_unit_double() {
            u64 max = std::numeric_limits<u64>::max();
            u64 val = gen_bits();
            return static_cast<double>( val ) / static_cast<double>( max );
        }

        /**
         * Generate integer in bounds [lo, hi]
         */
        constexpr i64 gen_int( i64 lo, i64 hi ) {
            if( hi < lo ) {
                return gen_int( hi, lo );
            } else {
                u64 bound = hi - lo + 1;
                u64 rem = engine() % bound;
                return lo + static_cast<i64>( rem );
            }
        }

        /**
         * Generate double in bounds [lo, hi]
         */
        constexpr double gen_double( double lo, double hi ) {
            if( hi < lo ) {
                return gen_double( hi, lo );
            } else {
                double diff = hi - lo;
                double mul = gen_unit_double();
                return lo + mul * diff;
            }
        }

        /**
         * Generate integer in bounds [lo, hi]
         */
        constexpr u64 gen_uint( u64 lo, u64 hi ) {
            if( hi < lo ) {
                return gen_uint( hi, lo );
            } else {
                u64 bound = hi - lo + 1;
                u64 rem = engine() % bound;
                return lo + rem;
            }
        }

        /**
         * Advance the generator by `num_steps` steps
         */
        constexpr void advance( u64 num_steps ) {
            for( u64 i = 0; i < num_steps; i++ ) {
                engine();
            }
        }

        /**
         * Returns a random entry in the container.
         * The container must have a `size()` function and must support iterators as usual.
         * For empty containers it returns the given default object.
         * `C` is the container type,
         * `D` is the type of the default value (which may differ from the return type)
         * `V` is the type of the elements in the container.
         * Note that this function does not return a reference because the default value could be
         * a temporary object that is not valid after this function has left:
         * \code random_entry( vect, std::string("default") ); \endcode
         */
        template<typename C, typename D, typename V = typename C::value_type>
        inline V random_entry( const C &container, D default_value ) {
            if( container.empty() ) {
                return default_value;
            }
            auto iter = container.begin();
            std::advance( iter, gen_uint( 0, container.size() - 1 ) );
            return *iter;
        }

        /**
         * Return a random entry in the container and remove it from the container.
         * The container must not be empty!
         */
        template<typename C, typename V = typename C::value_type>
        V random_entry_removed( C &container ) {
            auto iter = container.begin();
            std::advance( iter, gen_uint( 0, container.size() - 1 ) );
            const V result = std::move( *iter ); // Copy because the original is removed and thereby destroyed
            container.erase( iter );
            return result;
        }
};

u64 generate_seed_from_system();

#endif // CATA_SRC_RNG_SEEDABLE_H
