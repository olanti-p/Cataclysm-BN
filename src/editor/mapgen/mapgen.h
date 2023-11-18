#ifndef CATA_SRC_EDITOR_MAPGEN_H
#define CATA_SRC_EDITOR_MAPGEN_H

#include "common/canvas_2d.h"
#include "game_constants.h"
#include "coordinates.h"

#include "common/uuid.h"
#include "mapgen/palette.h"
#include "mapgen/mapobject.h"
#include "widget/editable_id.h"

struct ImVec4;
class JsonOut;
class JsonIn;
template<typename T> struct enum_traits;

namespace editor
{
struct State;

struct MapgenBase {
    MapgenBase() : canvas( point( SEEX * 2, SEEY * 2 ), UUID_INVALID ) { }
    ~MapgenBase();

    Canvas2D<UUID> canvas;
    UUID palette = UUID_INVALID;

    inline void set_size( point new_size ) {
        canvas.set_size( new_size, UUID_INVALID );
    }
    void remove_usages( const UUID &uuid );

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

enum class OterMapgenBase {
    FillTer,
    PredecessorMapgen,
    Rows,
    _Num,
};

struct MapgenOter {
    bool matrix_mode = true;
    std::vector<EID::OterType> om_terrain;
    Canvas2D<EID::OterType> om_terrain_matrix =
        Canvas2D<EID::OterType>( point( 1, 1 ), EID::OterType::NULL_ID() );
    int weight = 100;
    OterMapgenBase mapgen_base = OterMapgenBase::FillTer;
    EID::Ter fill_ter = EID::Ter::NULL_ID();
    EID::Oter predecessor_mapgen;
    IntRange rotation;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct MapgenUpdate {
    std::string update_mapgen_id;
    EID::Ter fill_ter = EID::Ter::NULL_ID();

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct MapgenNested {
    std::string nested_mapgen_id;
    point size = point( 24, 24 );
    IntRange rotation;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

enum class MapgenType {
    Oter,
    Update,
    Nested,
    _Num,
};

struct Mapgen {
    UUID uuid = UUID_INVALID;

    MapgenType mtype = MapgenType::Oter;
    MapgenBase base;
    MapgenOter oter;
    MapgenUpdate update;
    MapgenNested nested;

    std::vector<MapObject> objects;

    std::string name;

    std::string display_name() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    inline bool uses_rows() const {
        return mtype == editor::MapgenType::Nested ||
               (
                   mtype == editor::MapgenType::Oter &&
                   oter.mapgen_base == editor::OterMapgenBase::Rows
               );
    }

    point_rel_etile mapgensize() const;
};

/**
 * =============== Windows ===============
 */
void show_mapgen_info( State &state, Mapgen &mapgen, bool &show );

} // namespace editor

template<>
struct enum_traits<editor::OterMapgenBase> {
    static constexpr editor::OterMapgenBase last = editor::OterMapgenBase::_Num;
};

template<>
struct enum_traits<editor::MapgenType> {
    static constexpr editor::MapgenType last = editor::MapgenType::_Num;
};

#endif // CATA_SRC_EDITOR_MAPGEN_H
