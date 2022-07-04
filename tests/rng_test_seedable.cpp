#include "catch/catch.hpp"

#include "rng_seedable.h"

TEST_CASE( "RNG_deterministic", "[rng]" )
{
    // Should return same values on all platforms with all compilers
    RNG rng = RNG( 1337 );

    CHECK( rng.gen_bits() == 4248603512886213755 );
    CHECK( rng.gen_bits() == 3514442526888809492 );
    CHECK( rng.gen_bits() == 5734451162152304724 );
}

TEST_CASE( "RNG_save_load_state", "[rng]" )
{
    // Should return same values on all platforms with all compilers
    RNG rng = RNG( 1337 );

    CHECK( rng.gen_bits() == 4248603512886213755 );

    RNG rng_2;
    rng_2.load_from_state( rng.write_state() );

    CHECK( rng.gen_bits() == rng_2.gen_bits() );
    CHECK( rng.gen_bits() == rng_2.gen_bits() );
}

constexpr int NUM_ROLLS = 100000;
// Allow up to 0.5% deviation
constexpr float ROLL_EPSILON = 0.005f;

static void test_rng( float exp, std::function<bool()> f )
{
    int value = 0;
    for( int i = 0; i < NUM_ROLLS; i++ ) {
        if( f() ) {
            value++;
        }
    }
    float res = static_cast<float>( value ) / static_cast<float>( NUM_ROLLS );
    CHECK( res == Approx( exp ).epsilon( ROLL_EPSILON ) );
}

TEST_CASE( "RNG_api", "[rng]" )
{
    // Should return same values on all platforms with all compilers
    RNG rng = RNG( 1337 );

    SECTION( "gen_bool" ) {
        CHECK( !rng.gen_bool() );
        CHECK( rng.gen_bool() );
        CHECK( rng.gen_bool() );
        CHECK( rng.gen_bool() );
        CHECK( !rng.gen_bool() );
        CHECK( !rng.gen_bool() );
        CHECK( rng.gen_bool() );
        CHECK( !rng.gen_bool() );

        test_rng( 0.5f, [&]() {
            return rng.gen_bool();
        } );
    }

    SECTION( "one_in" ) {
        CHECK( rng.one_in( 4 ) );
        CHECK( rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );
        CHECK( !rng.one_in( 4 ) );

        test_rng( 0.25f, [&]() {
            return rng.one_in( 4 );
        } );
    }

    SECTION( "x_in_y" ) {
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( !rng.x_in_y( 2, 3 ) );
        CHECK( !rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );
        CHECK( rng.x_in_y( 2, 3 ) );

        test_rng( 0.6666f, [&]() {
            return rng.x_in_y( 2, 3 );
        } );
    }

    SECTION( "gen_bits" ) {
        CHECK( rng.gen_bits() == 0x3af61262890f3c7b );
        CHECK( rng.gen_bits() == 0x30c5cedb9ebbd414 );
        CHECK( rng.gen_bits() == 0x4f94dcf315ea0054 );
        CHECK( rng.gen_bits() == 0xe714b1448fcd744c );
    }

    SECTION( "gen_unit_double" ) {
        // TODO: these should not require Approx
        CHECK( rng.gen_unit_double() == Approx( 0.2303172579 ) );
        CHECK( rng.gen_unit_double() == Approx( 0.1905183111 ) );
        CHECK( rng.gen_unit_double() == Approx( 0.3108652204 ) );
        CHECK( rng.gen_unit_double() == Approx( 0.9026594918 ) );
        CHECK( rng.gen_unit_double() == Approx( 0.6849179080 ) );
        CHECK( rng.gen_unit_double() == Approx( 0.3078303993 ) );
    }

    SECTION( "gen_int" ) {
        bool hit_lo = false;
        bool hit_hi = false;
        constexpr int val_lo = -5;
        constexpr int val_hi = 15;

        for( int i = 0; i < NUM_ROLLS; i++ ) {
            int val = rng.gen_int( val_lo, val_hi );
            if( val < val_lo || val > val_hi ) {
                REQUIRE( val >= val_lo );
                REQUIRE( val <= val_hi );
            }
            hit_lo |= val == val_lo;
            hit_hi |= val == val_hi;
        }

        CHECK( hit_lo );
        CHECK( hit_hi );
    }

    SECTION( "gen_double" ) {
        constexpr double val_lo = -5.0;
        constexpr double val_hi = 15.0;

        for( int i = 0; i < NUM_ROLLS; i++ ) {
            double val = rng.gen_double( val_lo, val_hi );
            if( val < val_lo || val > val_hi ) {
                REQUIRE( val >= val_lo );
                REQUIRE( val <= val_hi );
            }
        }
        SUCCEED();
    }
}
