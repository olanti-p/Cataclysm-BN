#include "catch/catch.hpp"

#include "magic_enchantment.h"
#include "map.h"
#include "map_helpers.h"
#include "item.h"
#include "player.h"
#include "player_helpers.h"

static trait_id trait_CARNIVORE( "CARNIVORE" );
static efftype_id effect_debug_clairvoyance( "debug_clairvoyance" );

static void advance_turn( Character &guy )
{
    guy.recalculate_enchantment_cache();
    guy.process_turn();
    calendar::turn += 1_turns;
}

static void give_item( Character &guy, const std::string &item_id )
{
    guy.i_add( item( item_id ) );
}

static void clear_items( Character &guy )
{
    guy.inv.clear();
}

TEST_CASE( "Enchantments grant mutations", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    advance_turn( guy );

    REQUIRE( !guy.has_trait( trait_CARNIVORE ) );

    give_item( guy, "test_relic_gives_trait" );
    advance_turn( guy );

    CHECK( guy.has_trait( trait_CARNIVORE ) );

    clear_items( guy );
    advance_turn( guy );

    CHECK( !guy.has_trait( trait_CARNIVORE ) );
}

TEST_CASE( "Enchantments apply effects", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    advance_turn( guy );

    REQUIRE( !guy.has_effect( effect_debug_clairvoyance ) );

    give_item( guy, "architect_cube" );
    advance_turn( guy );

    CHECK( guy.has_effect( effect_debug_clairvoyance ) );

    clear_items( guy );
    advance_turn( guy );

    // FIXME: effects should go away after 1 turn!
    CHECK( guy.has_effect( effect_debug_clairvoyance ) );
    advance_turn( guy );

    CHECK( !guy.has_effect( effect_debug_clairvoyance ) );
}

TEST_CASE( "Enchantments modify stats", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    advance_turn( guy );

    REQUIRE( guy.get_str_base() == 8 );
    REQUIRE( guy.get_dex_base() == 8 );
    REQUIRE( guy.get_per_base() == 8 );
    REQUIRE( guy.get_int_base() == 8 );

    REQUIRE( guy.get_str() == 8 );
    REQUIRE( guy.get_dex() == 8 );
    REQUIRE( guy.get_per() == 8 );
    REQUIRE( guy.get_int() == 8 );

    give_item( guy, "test_relic_mods_stats" );
    advance_turn( guy );

    CHECK( guy.get_str_base() == 8 );
    CHECK( guy.get_dex_base() == 8 );
    CHECK( guy.get_per_base() == 8 );
    CHECK( guy.get_int_base() == 8 );

    CHECK( guy.get_str() == 20 );
    CHECK( guy.get_dex() == 6 );
    CHECK( guy.get_per() == 5 );
    CHECK( guy.get_int() == 0 );

    clear_items( guy );
    advance_turn( guy );

    CHECK( guy.get_str_base() == 8 );
    CHECK( guy.get_dex_base() == 8 );
    CHECK( guy.get_per_base() == 8 );
    CHECK( guy.get_int_base() == 8 );

    CHECK( guy.get_str() == 8 );
    CHECK( guy.get_dex() == 8 );
    CHECK( guy.get_per() == 8 );
    CHECK( guy.get_int() == 8 );
}

TEST_CASE( "Enchantments modify speed", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );
    const std::string s_relic = "test_relic_mods_speed";

    guy.set_moves( 0 );
    advance_turn( guy );

    REQUIRE( guy.get_speed_base() == 100 );
    REQUIRE( guy.get_speed() == 100 );
    REQUIRE( guy.get_moves() == 100 );

    give_item( guy, s_relic );
    guy.set_moves( 0 );
    advance_turn( guy );

    // FIXME: there's an extra turn delay between when speed bonus is applied and
    //        when it actually affects gained moves
    CHECK( guy.get_moves() == 100 );
    guy.set_moves( 0 );
    advance_turn( guy );

    CHECK( guy.get_speed_base() == 100 );
    CHECK( guy.get_speed() == 75 );
    CHECK( guy.get_moves() == 75 );

    clear_items( guy );
    guy.set_moves( 0 );
    advance_turn( guy );

    // FIXME: there's an extra turn delay between when speed bonus is applied and
    //        when it actually affects gained moves
    CHECK( guy.get_moves() == 75 );
    guy.set_moves( 0 );
    advance_turn( guy );

    CHECK( guy.get_speed_base() == 100 );
    CHECK( guy.get_speed() == 100 );
    CHECK( guy.get_moves() == 100 );

    guy.set_moves( 0 );
    guy.set_speed_base( 120 );
    for( int i = 0; i < 10; i++ ) {
        give_item( guy, s_relic );
    }

    advance_turn( guy );

    // FIXME: there's an extra turn delay between when speed bonus is applied and
    //        when it actually affects gained moves
    CHECK( guy.get_moves() == 120 );
    guy.set_moves( 0 );
    advance_turn( guy );

    CHECK( guy.get_speed_base() == 120 );
    CHECK( guy.get_speed() == 30 );
    CHECK( guy.get_moves() == 30 );
}
