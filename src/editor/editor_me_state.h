#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"
#include "../type_id.h"
#include "../mapgen.h"

#include "editor_assets.h"

struct ImDrawList;
struct ImVec4;

namespace editor
{

constexpr int MIN_SCALE = 8;
constexpr int MAX_SCALE = 128;
constexpr int DEFAULT_SCALE = 32;

struct me_camera {
    point_abs_epos pos;
    point_rel_epos drag_delta;
    int scale = DEFAULT_SCALE;

    point_abs_epos screen_to_world( const point_abs_screen &p ) const;
    point_abs_screen world_to_screen( const point_abs_epos &p ) const;
    point_rel_epos screen_to_world( const point_rel_screen &p ) const;
    point_rel_screen world_to_screen( const point_rel_epos &p ) const;
};

template<typename T>
struct editable_id {
    std::string data;

    editable_id() = default;
    editable_id( const editable_id<T> & ) = default;
    editable_id( editable_id<T> && ) = default;
    editable_id( const std::string &s ) : data( s ) {}
    editable_id( const string_id<T> &id ) : data( id.str() ) {}
    ~editable_id() = default;

    editable_id &operator= ( const editable_id<T> & ) = default;
    editable_id &operator= ( editable_id<T> && ) = default;

    bool is_valid() const {
        return string_id<T>( data ).is_valid();
    }

    const T &obj() const {
        return string_id<T>( data ).obj();
    }

    static const editable_id<T> NULL_ID() {
        return string_id<T>::NULL_ID();
    }
};

using ter_eid = editable_id<ter_t>;
using furn_eid = editable_id<furn_t>;
using oter_eid = editable_id<oter_t>;
using palette_eid = editable_id<mapgen_palette>;

struct me_placing {
    // TODO
    std::string dummy;
};

struct me_palette {
    static me_palette make_inline() {
        me_palette ret;
        ret.is_inline = true;
        return ret;
    }

    bool is_inline = false;
    palette_eid id;
    std::vector<std::pair<map_key, ter_eid>> terrain;
    std::vector<std::pair<map_key, furn_eid>> furniture;
    std::vector<std::pair<map_key, me_placing>> placings;
};

struct me_mapgen_base {
    std::vector<map_key> rows;
    me_palette inline_palette = me_palette::make_inline();
};

enum class OterMapgenBase {
    FillTer,
    PredecessorMapgen,
    Rows,
};

struct me_mapgen_oter {
    OterMapgenBase mapgen_base = OterMapgenBase::FillTer;
    ter_eid fill_ter = ter_eid::NULL_ID();
    oter_eid predecessor_mapgen;
    jmapgen_int rotation = jmapgen_int( 0 );
};

struct me_mapgen_update {
    ter_eid fill_ter = ter_eid::NULL_ID();
};

struct me_mapgen_nested {
    point size = point( 1, 1 );
    jmapgen_int rotation = jmapgen_int( 0 );
};

enum class MapgenType {
    Oter,
    Update,
    Nested,
};

struct me_file {
    MapgenType mtype = MapgenType::Oter;
    me_mapgen_base base;
    me_mapgen_oter oter;
    me_mapgen_update update;
    me_mapgen_nested nested;

    point_rel_etile mapgensize();
};

struct me_state {
    me_state();
    me_state( const me_state & ) = delete;
    me_state( me_state && ) = default;
    ~me_state();

    me_state &operator=( const me_state & ) = delete;
    me_state &operator=( me_state && ) = default;

    me_camera camera;
    bool do_loop = true; // Setting this to false will quit the editor
    bool show_demo_wnd = false; // Whether to show ImGui Demo window
    bool show_asset_lib = false; // Whether to show asset library
    bool show_file_info = true; // Whether to show file info
    bool show_base_inline_palette = false; // Whether to show base mapgen's palette
    asset_library assets;
    me_file file;
};

/**
 * ============ Mouse helpers ============
 */
point_abs_screen get_mouse_pos();
point_abs_etile get_mouse_tile_pos( const me_camera &cam );

/**
 * ========== Rendering helpers ==========
 */
void draw_frame(
    ImDrawList *draw_list,
    const me_camera &cam,
    const point_abs_etile &p1,
    const point_abs_etile &p2,
    ImVec4 col,
    bool filled
);
void highlight_tile(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile tile,
    ImVec4 col
);
void highlight_region(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col_bg,
    ImVec4 col_border
);

/**
 * =============== Windows ===============
 */
void show_canvas( me_state &state );
void show_control_window( me_state &state );
void show_asset_lib( asset_library &assets, bool &show );
void show_file_info( me_state &state, me_file &file, bool &show );
void show_palette( me_palette &p, bool &show );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H
