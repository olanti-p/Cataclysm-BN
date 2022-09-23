#pragma once
#ifndef CATA_SRC_MAPGEN_PIECE_H
#define CATA_SRC_MAPGEN_PIECE_H

#include "computer.h"
#include "field_type.h"
#include "item_group.h"
#include "mapgen.h"
#include "mapgendata.h"
#include "map.h"
#include "vpart_position.h"
#include "om_direction.h"
#include "rng.h"

class npc_template;

enum class JmPieceType : int {
    Field = 0,
    NPC,
    Faction,
    Sign,
    Graffiti,
    VendingMachine,
    Toilet,
    GasPump,
    Liquid,
    Igroup,
    Loot,
    Mgroup,
    Monster,
    Vehicle,
    Item,
    Trap,
    Furniture,
    Terrain,
    TerFurnTransform,
    MakeRubble,
    Computer,
    SealedItem,
    Translate,
    Zone,
    Nested,
    AltTrap,
    AltFurniture,
    AltTerrain,

    NumJmTypes
};

/**
 * Places fields on the map.
 * "field": field type ident.
 * "intensity": initial field intensity.
 * "age": initial field age.
 */
class jmapgen_field : public jmapgen_piece
{
    public:
        field_type_id ftype;
        int intensity;
        time_duration age;
        jmapgen_field( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Field;
        }
};

/**
 * Place an NPC.
 * "class": the npc class, see @ref map::place_npc
 */
class jmapgen_npc : public jmapgen_piece
{
    public:
        string_id<npc_template> npc_class;
        bool target;
        std::vector<std::string> traits;
        jmapgen_npc( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::NPC;
        }
};

/**
* Place ownership area
*/
class jmapgen_faction : public jmapgen_piece
{
    public:
        faction_id id;
        jmapgen_faction( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Faction;
        }
};

/**
 * Place a sign with some text.
 * "signage": the text on the sign.
 */
class jmapgen_sign : public jmapgen_piece
{
    public:
        std::string signage;
        std::string snippet;
        jmapgen_sign( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        std::string apply_all_tags( std::string signtext, const std::string &cityname ) const;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Sign;
        }
};

/**
 * Place graffiti with some text or a snippet.
 * "text": the text of the graffiti.
 * "snippet": snippet category to pull from for text instead.
 */
class jmapgen_graffiti : public jmapgen_piece
{
    public:
        std::string text;
        std::string snippet;
        jmapgen_graffiti( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        std::string apply_all_tags( std::string graffiti, const std::string &cityname ) const;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Graffiti;
        }
};

/**
 * Place a vending machine with content.
 * "item_group": the item group that is used to generate the content of the vending machine.
 */
class jmapgen_vending_machine : public jmapgen_piece
{
    public:
        bool reinforced;
        item_group_id item_group;
        jmapgen_vending_machine( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::VendingMachine;
        }
};
/**
 * Place a toilet with (dirty) water in it.
 * "amount": number of water charges to place.
 */
class jmapgen_toilet : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        jmapgen_toilet( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Toilet;
        }
};
/**
 * Place a gas pump with fuel in it.
 * "amount": number of fuel charges to place.
 */
class jmapgen_gaspump : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        std::string fuel;
        jmapgen_gaspump( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::GasPump;
        }
};

/**
 * Place a specific liquid into the map.
 * "liquid": id of the liquid item (item should use charges)
 * "amount": quantity of liquid placed (a value of 0 uses the default amount)
 * "chance": chance of liquid being placed, see @ref map::place_items
 */
class jmapgen_liquid_item : public jmapgen_piece
{
    public:
        jmapgen_int amount;
        itype_id liquid;
        jmapgen_int chance;
        jmapgen_liquid_item( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Liquid;
        }
};

/**
 * Place items from an item group.
 * "item": id of the item group.
 * "chance": chance of items being placed, see @ref map::place_items
 * "repeat": number of times to apply this piece
 */
class jmapgen_item_group : public jmapgen_piece
{
    public:
        item_group_id group_id;
        jmapgen_int chance;
        jmapgen_item_group( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Igroup;
        }
};

/** Place items from an item group */
class jmapgen_loot : public jmapgen_piece
{
    public:
        jmapgen_loot( const JsonObject &jsi );

        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Loot;
        }

        Item_group result_group;
        int chance;
};

/**
 * Place spawn points for a monster group (actual monster spawning is done later).
 * "monster": id of the monster group.
 * "chance": see @ref map::place_spawns
 * "density": see @ref map::place_spawns
 */
class jmapgen_monster_group : public jmapgen_piece
{
    public:
        mongroup_id id;
        float density;
        jmapgen_int chance;
        jmapgen_monster_group( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Mgroup;
        }
};
/**
 * Place spawn points for a specific monster.
 * "monster": id of the monster. or "group": id of the monster group.
 * "friendly": whether the new monster is friendly to the player character.
 * "name": the name of the monster (if it has one).
 * "chance": the percentage chance of a monster, affected by spawn density
 *     If high density means greater than one hundred percent, can place multiples.
 * "repeat": roll this many times for creatures, potentially spawning multiples.
 * "pack_size": place this many creatures each time a roll is successful.
 * "one_or_none": place max of 1 (or pack_size) monsters, even if spawn density > 1.
 *     Defaults to true if repeat and pack_size are unset, false if one is set.
 */
class jmapgen_monster : public jmapgen_piece
{
    public:
        weighted_int_list<mtype_id> ids;
        mongroup_id m_id = mongroup_id::NULL_ID();
        jmapgen_int chance;
        jmapgen_int pack_size;
        bool one_or_none;
        bool friendly;
        std::string name;
        bool target;
        jmapgen_monster( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Monster;
        }
};

/**
 * Place a vehicle.
 * "vehicle": id of the vehicle.
 * "chance": chance of spawning the vehicle: 0...100
 * "rotation": rotation of the vehicle, see @ref vehicle::vehicle
 * "fuel": fuel status of the vehicle, see @ref vehicle::vehicle
 * "status": overall (damage) status of the vehicle, see @ref vehicle::vehicle
 */
class jmapgen_vehicle : public jmapgen_piece
{
    public:
        vgroup_id type;
        jmapgen_int chance;
        std::vector<units::angle> rotation;
        int fuel;
        int status;
        jmapgen_vehicle( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Vehicle;
        }
};
/**
 * Place a specific item.
 * "item": id of item type to spawn.
 * "chance": chance of spawning it (1 = always, otherwise one_in(chance)).
 * "amount": amount of items to spawn.
 * "repeat": number of times to apply this piece
 */
class jmapgen_spawn_item : public jmapgen_piece
{
    public:
        itype_id type;
        jmapgen_int amount;
        jmapgen_int chance;
        jmapgen_spawn_item( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Item;
        }
};
/**
 * Place a trap.
 * "trap": id of the trap.
 */
class jmapgen_trap : public jmapgen_piece
{
    public:
        trap_id id;
        jmapgen_trap( const JsonObject &jsi );

        jmapgen_trap( const std::string &tid );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Trap;
        }
};
/**
 * Place a furniture.
 * "furn": id of the furniture.
 */
class jmapgen_furniture : public jmapgen_piece
{
    public:
        furn_id id;
        jmapgen_furniture( const JsonObject &jsi );
        jmapgen_furniture( const std::string &fid );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Furniture;
        }
};
/**
 * Place terrain.
 * "ter": id of the terrain.
 */
class jmapgen_terrain : public jmapgen_piece
{
    public:
        ter_id id;
        jmapgen_terrain( const JsonObject &jsi );
        jmapgen_terrain( const std::string &tid );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Terrain;
        }
};
/**
 * Run a transformation.
 * "transform": id of the ter_furn_transform to run.
 */
class jmapgen_ter_furn_transform: public jmapgen_piece
{
    public:
        ter_furn_transform_id id;
        jmapgen_ter_furn_transform( const JsonObject &jsi );
        jmapgen_ter_furn_transform( const std::string &rid );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::TerFurnTransform;
        }
};
/**
 * Calls @ref map::make_rubble to create rubble and destroy the existing terrain/furniture.
 * See map::make_rubble for explanation of the parameters.
 */
class jmapgen_make_rubble : public jmapgen_piece
{
    public:
        furn_id rubble_type = f_rubble;
        bool items = false;
        ter_id floor_type = t_dirt;
        bool overwrite = false;
        jmapgen_make_rubble( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::MakeRubble;
        }
};

/**
 * Place a computer (console) with given stats and effects.
 * @param options Array of @ref computer_option
 * @param failures Array of failure effects (see @ref computer_failure)
 */
class jmapgen_computer : public jmapgen_piece
{
    public:
        translation name;
        translation access_denied;
        int security;
        std::vector<computer_option> options;
        std::vector<computer_failure> failures;
        bool target;
        jmapgen_computer( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Computer;
        }
};

/**
 * Place an item in furniture (expected to be used with NOITEM SEALED furniture like plants).
 * "item": item to spawn (object with usual parameters).
 * "items": item group to spawn (object with usual parameters).
 * "furniture": furniture to create around it.
 */
class jmapgen_sealed_item : public jmapgen_piece
{
    public:
        furn_id furniture;
        jmapgen_int chance;
        cata::optional<jmapgen_spawn_item> item_spawner;
        cata::optional<jmapgen_item_group> item_group_spawner;
        jmapgen_sealed_item( const JsonObject &jsi );

        void check( const std::string &oter_name ) const override;

        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::SealedItem;
        }
};
/**
 * Translate terrain from one ter_id to another.
 * "from": id of the starting terrain.
 * "to": id of the ending terrain
 * not useful for normal mapgen, very useful for mapgen_update
 */
class jmapgen_translate : public jmapgen_piece
{
    public:
        ter_id from;
        ter_id to;
        jmapgen_translate( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &/*x*/, const jmapgen_int &/*y*/ ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Translate;
        }
};
/**
 * Place a zone
 */
class jmapgen_zone : public jmapgen_piece
{
    public:
        zone_type_id zone_type;
        faction_id faction;
        std::string name = "";
        jmapgen_zone( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Zone;
        }
};

class neighborhood_check
{
    public:
        // To speed up the most common case: no checks
        bool has_any = false;
        std::array<std::set<oter_str_id>, om_direction::size> neighbors;
        std::set<oter_str_id> above;

        neighborhood_check( const JsonObject &jsi );

        bool test( mapgendata &dat ) const;
};

/**
 * Calls another mapgen call inside the current one.
 * Note: can't use regular overmap ids.
 * @param entries list of pairs [nested mapgen id, weight].
 */
class jmapgen_nested : public jmapgen_piece
{
    public:
        weighted_int_list<std::string> entries;
        weighted_int_list<std::string> else_entries;
        neighborhood_check neighbors;
        jmapgen_nested( const JsonObject &jsi );
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override;
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override;
        void show_details() const override;
        JmPieceType get_type() const override {
            return JmPieceType::Nested;
        }
};

/**
 * This is a generic mapgen piece, the template parameter PieceType should be another specific
 * type of jmapgen_piece. This class contains a vector of those objects and will chose one of
 * it at random.
 */
template<typename PieceType>
class jmapgen_alternativly : public jmapgen_piece
{
    public:
        // Note: this bypasses virtual function system, all items in this vector are of type
        // PieceType, they *can not* be of any other type.
        std::vector<PieceType> alternatives;
        jmapgen_alternativly() = default;
        void apply( mapgendata &dat, const jmapgen_int &x, const jmapgen_int &y ) const override {
            if( const auto chosen = random_entry_opt( alternatives ) ) {
                chosen->get().apply( dat, x, y );
            }
        }
        bool has_vehicle_collision( mapgendata &dat, const point &p ) const override {
            return dat.m.veh_at( tripoint( p, dat.zlevel() ) ).has_value();
        }
        void show_details() const override;
        JmPieceType get_type() const override;
};

using jmapgen_alternativly_trap = jmapgen_alternativly<jmapgen_trap>;
using jmapgen_alternativly_furniture = jmapgen_alternativly<jmapgen_furniture>;
using jmapgen_alternativly_terrain = jmapgen_alternativly<jmapgen_terrain>;

#endif // CATA_SRC_MAPGEN_PIECE_H
