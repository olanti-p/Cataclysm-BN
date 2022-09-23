#ifndef CATA_SRC_EDITOR_EDITOR_ME_STATE_H
#define CATA_SRC_EDITOR_EDITOR_ME_STATE_H

#include "../options.h"
#include "../coordinates.h"
#include "../type_id.h"
#include "../mapgen.h"

#include "editor_assets.h"
#include "imgui.h"

struct ImDrawList;
struct ImVec4;
class JsonOut;
class JsonIn;
template<typename T> struct enum_traits;

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

namespace detail
{
void serialize_eid( JsonOut &jsout, const std::string &data );
void deserialize_eid( JsonIn &jsin, std::string &data );
} // namespace detail

template<typename T>
struct editable_id {
    public:
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

        bool is_null() const {
            return string_id<T>( data ).is_null();
        }

        const T &obj() const {
            return string_id<T>( data ).obj();
        }

        static const editable_id<T> NULL_ID() {
            return string_id<T>::NULL_ID();
        }

        static const std::vector<std::string> &get_all_opts();

        void serialize( JsonOut &jsout ) const {
            detail::serialize_eid( jsout, data );
        }
        void deserialize( JsonIn &jsin ) {
            detail::deserialize_eid( jsin, data );
        }

    private:
        // TODO: invalidate on data change
        static std::vector<std::string> all_opts;
};

template<typename T>
std::vector<std::string> editable_id<T>::all_opts;

using ter_eid = editable_id<ter_t>;
using furn_eid = editable_id<furn_t>;
using oter_eid = editable_id<oter_t>;
using palette_eid = editable_id<mapgen_palette>;

using uuid_t = uint64_t;
constexpr uuid_t UUID_INVALID = 0;

struct uuid_generator {
    private:
        uuid_t counter = UUID_INVALID;

    public:
        inline uuid_t operator()() {
            counter++;
            return counter;
        }

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
};

struct me_map_key_generator {
    private:
        std::vector<map_key> opts;

    public:
        me_map_key_generator();
        ~me_map_key_generator() = default;

        void blacklist( const map_key &opt );

        inline map_key operator()() {
            if( opts.empty() ) {
                return default_map_key;
            } else {
                return opts[0];
            }
        }
};

struct me_int_range {
    int min = 0;
    int max = 0;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_placing {
    // TODO
    std::string dummy;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_palette_entry {
    uuid_t uuid;
    map_key key;
    ImVec4 color;
    ter_eid ter;
    furn_eid furn;
    me_placing placing;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_palette {
    static me_palette make_inline() {
        me_palette ret;
        ret.is_inline = true;
        return ret;
    }

    bool is_inline = false;
    palette_eid id;
    std::vector<me_palette_entry> entries;

    const map_key &key_from_uuid( const uuid_t &uuid ) const;
    const ImVec4 &color_from_uuid( const uuid_t &uuid ) const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_mapgen_base {
    me_mapgen_base() {
        set_size( point( SEEX * 2, SEEY * 2 ) );
    }
    ~me_mapgen_base();

    point size;
    // TODO: refer to palette entries by their ids
    std::vector<uuid_t> rows;
    me_palette inline_palette = me_palette::make_inline();

    void set_size( const point &s );
    inline void set_uuid_at( const point &pos, const uuid_t &uuid ) {
        rows[ pos.y * size.x + pos.x ] = uuid;
    }
    inline const uuid_t &get_uuid_at( const point &pos ) const {
        return rows[ pos.y * size.x + pos.x ];
    }
    inline const map_key &get_key_at( const point &pos ) const {
        return inline_palette.key_from_uuid( get_uuid_at( pos ) );
    }
    inline const ImVec4 &get_color_at( const point &pos ) const {
        return inline_palette.color_from_uuid( get_uuid_at( pos ) );
    }
    map_key pick_available_key() const;
    void remove_usages( const uuid_t &uuid );

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

enum class OterMapgenBase {
    FillTer,
    PredecessorMapgen,
    Rows,
    _Num,
};

struct me_mapgen_oter {
    OterMapgenBase mapgen_base = OterMapgenBase::FillTer;
    ter_eid fill_ter = ter_eid::NULL_ID();
    oter_eid predecessor_mapgen;
    me_int_range rotation;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_mapgen_update {
    ter_eid fill_ter = ter_eid::NULL_ID();

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

struct me_mapgen_nested {
    point size = point( 1, 1 );
    me_int_range rotation;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

enum class MapgenType {
    Oter,
    Update,
    Nested,
    _Num,
};

struct me_file {
    uuid_generator uuid_gen;

    MapgenType mtype = MapgenType::Oter;
    me_mapgen_base base;
    me_mapgen_oter oter;
    me_mapgen_update update;
    me_mapgen_nested nested;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    point_rel_etile mapgensize() const;
};

struct me_file_revision {
    std::unique_ptr<me_file> file;
    int num = 0;

    me_file_revision() {
        file = std::make_unique<me_file>();
    }
    me_file_revision( const me_file_revision & ) = delete;
    me_file_revision( me_file_revision && ) = default;
    ~me_file_revision() {};

    me_file_revision &operator=( const me_file_revision & ) = delete;
    me_file_revision &operator=( me_file_revision && ) = default;

    me_file_revision make_copy() const {
        me_file_revision ret;
        ret.file = std::make_unique<me_file>( *file );
        ret.num = num;
        return ret;
    }
};

struct me_state {
    me_state();
    explicit me_state( std::unique_ptr<me_file> &&file );
    me_state( std::unique_ptr<me_file> &&file, const std::string *loaded_from_path );
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
    bool show_file_history = true; // Whether to show undo/redo history
    asset_library assets;

    bool ongoing_brush_stroke = false;
    bool brush_stroke_changed_data = false;

    bool open_save_as = false;
    bool do_save = false;
    bool do_exit_after_save = false;
    cata::optional<std::string> file_save_path;

    bool open_export_as = false;
    bool do_export = false;
    cata::optional<std::string> file_export_path;

    inline me_file &file() {
        return *current_revision.file;
    }

    inline void mark_changed() {
        file_has_changes = true;
    }

    inline bool can_undo() const {
        return current_revision.num != file_history[file_history.size() - 1].num;
    }

    inline void queue_undo() {
        switch_to_revision = current_revision.num - 1;
    }

    inline bool can_redo() const {
        return current_revision.num != file_history[0].num;
    }

    inline void queue_redo() {
        switch_to_revision = current_revision.num + 1;
    }

    bool has_unsaved_changes() const;
    bool has_unexported_changes() const;

    bool file_has_changes = false;
    cata::optional<int> switch_to_revision;
    me_file_revision current_revision;
    std::vector<me_file_revision> file_history;
    int history_capacity = 200;
    cata::optional<int> last_saved_revision;
    cata::optional<int> last_exported_revision;

    uuid_t rows_brush = UUID_INVALID;
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
void fill_tile(
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
void fill_region(
    ImDrawList *draw_list,
    const me_camera &cam,
    point_abs_etile p1,
    point_abs_etile p2,
    ImVec4 col
);

/**
 * =============== Windows ===============
 */
void show_canvas( me_state &state );
void show_control_window( me_state &state );
void show_file_history( me_state &state, bool &show );
void show_asset_lib( asset_library &assets, bool &show );
void show_file_info( me_state &state, me_file &file, bool &show );
void show_palette( me_state &state, me_palette &p, bool &show );

/**
 * ============= Entry point =============
 */
void show_me_ui( me_state &state );

} // namespace editor

template<>
struct enum_traits<editor::OterMapgenBase> {
    static constexpr editor::OterMapgenBase last = editor::OterMapgenBase::_Num;
};

template<>
struct enum_traits<editor::MapgenType> {
    static constexpr editor::MapgenType last = editor::MapgenType::_Num;
};

#endif // CATA_SRC_EDITOR_EDITOR_ME_STATE_H
