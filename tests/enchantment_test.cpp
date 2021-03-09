#include "catch/catch.hpp"

#include "magic.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_helpers.h"
#include "item.h"
#include "options.h"
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

TEST_CASE( "Enchantments modify attack cost", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );

    item normal_sword( "test_normal_sword" );
    item relic_sword( "test_relic_sword" );

    REQUIRE( normal_sword.attack_cost() == 101 );
    CHECK( relic_sword.attack_cost() == 86 );

    advance_turn( guy );

    REQUIRE( guy.attack_cost( normal_sword ) == 96 );
    CHECK( guy.attack_cost( relic_sword ) == 82 );

    give_item( guy, "test_relic_mods_atk_cost" );
    guy.recalculate_enchantment_cache();

    CHECK( guy.attack_cost( normal_sword ) == 77 );
    CHECK( guy.attack_cost( relic_sword ) == 66 );

    clear_items( guy );
    guy.recalculate_enchantment_cache();

    CHECK( guy.attack_cost( normal_sword ) == 96 );
    CHECK( guy.attack_cost( relic_sword ) == 82 );

    for( int i = 0; i < 10; i++ ) {
        give_item( guy, "test_relic_mods_atk_cost" );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.attack_cost( normal_sword ) == 25 );
    CHECK( guy.attack_cost( relic_sword ) == 25 );
}

TEST_CASE( "Enchantments modify move cost", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );
    guy.set_mutation( trait_id( "PADDED_FEET" ) );
    const std::string s_relic = "test_relic_mods_mv_cost";

    advance_turn( guy );

    REQUIRE( guy.run_cost( 100 ) == 90 );

    give_item( guy, s_relic );
    guy.recalculate_enchantment_cache();

    CHECK( guy.run_cost( 100 ) == 81 );

    for( int i = 0; i < 13; i++ ) {
        give_item( guy, s_relic );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.run_cost( 100 ) == 20 );

    clear_items( guy );
    guy.recalculate_enchantment_cache();

    CHECK( guy.run_cost( 100 ) == 90 );
}

TEST_CASE( "Enchantments modify metabolic rate", "[magic][enchantment][metabolism]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );
    const std::string s_relic = "test_relic_mods_metabolism";

    advance_turn( guy );

    const float normal_mr = get_option<float>( "PLAYER_HUNGER_RATE" );
    REQUIRE( guy.metabolic_rate_base() == normal_mr );
    REQUIRE( normal_mr == 1.0f );

    guy.set_mutation( trait_id( "HUNGER" ) );

    REQUIRE( guy.metabolic_rate_base() == Approx( 1.5f ) );

    give_item( guy, s_relic );
    guy.recalculate_enchantment_cache();

    CHECK( guy.metabolic_rate_base() == Approx( 1.35f ) );

    for( int i = 0; i < 11; i++ ) {
        give_item( guy, s_relic );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.metabolic_rate_base() == Approx( 0.0f ) );

    clear_items( guy );
    guy.recalculate_enchantment_cache();

    CHECK( guy.metabolic_rate_base() == Approx( 1.5f ) );
}

TEST_CASE( "Enchantments modify mana pool", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );
    guy.set_max_power_level( 100_kJ );
    guy.set_power_level( 0_kJ );
    REQUIRE( guy.get_max_power_level() == 100_kJ );
    REQUIRE( guy.get_power_level() == 0_kJ );
    const std::string s_relic = "test_relic_mods_manapool";

    advance_turn( guy );
    guy.magic->set_mana( 0 );

    // Unmodified mana pool should replenish in 8 hours
    REQUIRE( guy.magic->max_mana( guy ) == 1000 );
    constexpr double normal_regen_rate = 1000.0 / to_turns<double>( 8_hours );

    REQUIRE( guy.magic->available_mana() == 0 );
    REQUIRE( guy.magic->mana_regen_rate( guy ) == Approx( normal_regen_rate ) );

    give_item( guy, s_relic );
    guy.recalculate_enchantment_cache();

    CHECK( guy.magic->max_mana( guy ) == 800 );
    // 0.8 since regen rate scales with capacity; 0.7 is from MANA_REGEN modifier
    CHECK( guy.magic->mana_regen_rate( guy ) == Approx( normal_regen_rate * 0.8 * 0.7 ) );

    // bionic power penatly still works
    guy.set_power_level( 77_kJ );
    REQUIRE( guy.get_power_level() == 77_kJ );
    CHECK( guy.magic->max_mana( guy ) == 723 );
    guy.set_power_level( 0_kJ );
    REQUIRE( guy.get_power_level() == 0_kJ );
    CHECK( guy.magic->max_mana( guy ) == 800 );

    for( int i = 0; i < 3; i++ ) {
        give_item( guy, s_relic );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.magic->max_mana( guy ) == 200 );
    CHECK( guy.magic->mana_regen_rate( guy ) == Approx( 0.0 ) );

    for( int i = 0; i < 3; i++ ) {
        give_item( guy, s_relic );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.magic->max_mana( guy ) == 0 );
    CHECK( guy.magic->mana_regen_rate( guy ) == Approx( 0.0 ) );

    clear_items( guy );
    guy.recalculate_enchantment_cache();

    CHECK( guy.magic->available_mana() == 0 );
    CHECK( guy.magic->mana_regen_rate( guy ) == Approx( normal_regen_rate ) );
}

TEST_CASE( "Enchantments modify bionic power capacity", "[magic][enchantment]" )
{
    clear_map();
    Character &guy = get_player_character();
    clear_character( *guy.as_player(), true );
    guy.set_max_power_level( 100_kJ );
    REQUIRE( guy.get_max_power_level() == 100_kJ );
    const std::string s_relic = "test_relic_mods_bp_cap";

    advance_turn( guy );

    REQUIRE( guy.get_max_power_level() == 100_kJ );

    give_item( guy, s_relic );
    guy.recalculate_enchantment_cache();

    CHECK( guy.get_max_power_level() == 80_kJ );

    for( int i = 0; i < 7; i++ ) {
        give_item( guy, s_relic );
    }
    guy.recalculate_enchantment_cache();

    CHECK( guy.get_max_power_level() == 0_kJ );

    clear_items( guy );
    guy.recalculate_enchantment_cache();

    CHECK( guy.get_max_power_level() == 100_kJ );

    guy.set_max_power_level( 1500_kJ );
    REQUIRE( guy.get_max_power_level() == 1500_kJ );

    give_item( guy, s_relic );
    guy.recalculate_enchantment_cache();

    CHECK( guy.get_max_power_level() == units::energy_max );
}
