#include "editor_me_state.h"

#include "../json.h"
#include "../mapgen.h"

void serialize( const map_key &mk, JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "str", mk.str );
    jsout.end_object();
}

void deserialize( map_key &mk, JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "str", mk.str );
}

void serialize( const ImVec4 &v, JsonOut &jsout )
{
    jsout.start_object();
    jsout.member( "x", v.x );
    jsout.member( "y", v.y );
    jsout.member( "z", v.z );
    jsout.member( "w", v.w );
    jsout.end_object();
}

void deserialize( ImVec4 &v, JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "x", v.x );
    jo.read( "y", v.y );
    jo.read( "z", v.z );
    jo.read( "w", v.w );
}

namespace io
{

template<>
std::string enum_to_string<editor::OterMapgenBase>( editor::OterMapgenBase data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::OterMapgenBase::FillTer: return "FillTer";
        case editor::OterMapgenBase::PredecessorMapgen: return "PredecessorMapgen";
        case editor::OterMapgenBase::Rows: return "Rows";
        // *INDENT-ON*
        case editor::OterMapgenBase::_Num:
            break;
    }
    debugmsg( "Invalid editor::OterMapgenBase" );
    abort();
}

template<>
std::string enum_to_string<editor::MapgenType>( editor::MapgenType data )
{
    switch( data ) {
        // *INDENT-OFF*
        case editor::MapgenType::Nested: return "Nested";
        case editor::MapgenType::Oter: return "Oter";
        case editor::MapgenType::Update: return "Update";
        // *INDENT-ON*
        case editor::MapgenType::_Num:
            break;
    }
    debugmsg( "Invalid editor::MapgenType" );
    abort();
}

} // namespace io

namespace editor
{

namespace detail
{

void serialize_eid( JsonOut &jsout, const std::string &data )
{
    jsout.start_object();
    jsout.member( "data", data );
    jsout.end_object();
}

void deserialize_eid( JsonIn &jsin, std::string &data )
{
    JsonObject jo = jsin.get_object();

    jo.read( "data", data );
}

} // namespace detail

void uuid_generator::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "counter", counter );
    jsout.end_object();
}

void uuid_generator::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "counter", counter );
}

void me_int_range::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "min", min );
    jsout.member( "max", max );
    jsout.end_object();
}

void me_int_range::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "min", min );
    jo.read( "max", max );
}

void me_placing::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "dummy", dummy );
    jsout.end_object();
}

void me_placing::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "dummy", dummy );
}

void me_palette_entry::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "uuid", uuid );
    jsout.member( "key", key );
    jsout.member( "color", color );
    jsout.member( "ter", ter );
    jsout.member( "furn", furn );
    jsout.member( "placing", placing );
    jsout.end_object();
}

void me_palette_entry::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "uuid", uuid );
    jo.read( "key", key );
    jo.read( "color", color );
    jo.read( "ter", ter );
    jo.read( "furn", furn );
    jo.read( "placing", placing );
}

void me_palette::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "is_inline", is_inline );
    jsout.member( "id", id );
    jsout.member( "entries", entries );
    jsout.end_object();
}

void me_palette::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "is_inline", is_inline );
    jo.read( "id", id );
    jo.read( "entries", entries );
}

void me_mapgen_base::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "size", size );
    jsout.member( "rows", rows );
    jsout.member( "inline_palette", inline_palette );
    jsout.end_object();
}

void me_mapgen_base::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "size", size );
    jo.read( "rows", rows );
    jo.read( "inline_palette", inline_palette );
}

void me_mapgen_oter::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "om_terrain", om_terrain );
    jsout.member( "weight", weight );
    jsout.member( "mapgen_base", mapgen_base );
    jsout.member( "fill_ter", fill_ter );
    jsout.member( "predecessor_mapgen", predecessor_mapgen );
    jsout.member( "rotation", rotation );
    jsout.end_object();
}

void me_mapgen_oter::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "om_terrain", om_terrain );
    jo.read( "weight", weight );
    jo.read( "mapgen_base", mapgen_base );
    jo.read( "fill_ter", fill_ter );
    jo.read( "predecessor_mapgen", predecessor_mapgen );
    jo.read( "rotation", rotation );
}

void me_mapgen_update::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "update_mapgen_id", update_mapgen_id );
    jsout.member( "fill_ter", fill_ter );
    jsout.end_object();
}

void me_mapgen_update::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "update_mapgen_id", update_mapgen_id );
    jo.read( "fill_ter", fill_ter );
}

void me_mapgen_nested::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "nested_mapgen_id", nested_mapgen_id );
    jsout.member( "size", size );
    jsout.member( "rotation", rotation );
    jsout.end_object();
}

void me_mapgen_nested::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "nested_mapgen_id", nested_mapgen_id );
    jo.read( "size", size );
    jo.read( "rotation", rotation );
}

void me_file::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "uuid_gen", uuid_gen );
    jsout.member( "mtype", mtype );
    jsout.member( "base", base );
    jsout.member( "oter", oter );
    jsout.member( "update", update );
    jsout.member( "nested", nested );
    jsout.end_object();
}

void me_file::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "uuid_gen", uuid_gen );
    jo.read( "mtype", mtype );
    jo.read( "base", base );
    jo.read( "oter", oter );
    jo.read( "update", update );
    jo.read( "nested", nested );
}

} // namespace editor
