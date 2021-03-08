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
