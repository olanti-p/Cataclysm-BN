#include "editor_me_state_export.h"

#include "../fstream_utils.h"
#include "../json.h"
#include "../../tools/format/format.h"

#include <sstream>

namespace editor_export
{

/**
 * ============= EMIT DECLARATIONS =============
 */

void emit_key( JsonOut &jo, const std::string &key );

void emit_val( JsonOut &jo, const int &i );
void emit_val( JsonOut &jo, const std::string &str );
template<typename T>
void emit_val( JsonOut &jo, const editor::editable_id<T> &eid );
void emit_val( JsonOut &jo, const editor::me_int_range &r );

template<typename T>
void emit( JsonOut &jo, const std::string &key, const T &value );

template<typename F>
void emit_array( JsonOut &jo, F func );
template<typename F>
void emit_array( JsonOut &jo, const std::string &key, F func );

template<typename F>
void emit_object( JsonOut &jo, F func );
template<typename F>
void emit_object( JsonOut &jo, const std::string &key, F func );

/**
 * ============= EMIT DEFINITIONS =============
 */

void emit_key( JsonOut &jo, const std::string &key )
{
    jo.member( key );
}

void emit_val( JsonOut &jo, const int &i )
{
    jo.write( i );
}

void emit_val( JsonOut &jo, const std::string &str )
{
    jo.write( str );
}

template<typename T>
void emit_val( JsonOut &jo, const editor::editable_id<T> &eid )
{
    jo.write( eid.data );
}

void emit_val( JsonOut &jo, const editor::me_int_range &r )
{
    if( r.min == r.max ) {
        emit_val( jo, r.min );
    } else {
        emit_array( jo, [&]() {
            emit_val( jo, r.min );
            emit_val( jo, r.max );
        } );
    }
}

template<typename T>
void emit( JsonOut &jo, const std::string &key, const T &value )
{
    emit_key( jo, key );
    emit_val( jo, value );
}

template<typename F>
void emit_array( JsonOut &jo, F func )
{
    jo.start_array();
    func();
    jo.end_array();
}

template<typename F>
void emit_array( JsonOut &jo, const std::string &key, F func )
{
    emit_key( jo, key );
    emit_array( jo, func );
}

template<typename F>
void emit_object( JsonOut &jo, F func )
{
    jo.start_object();
    func();
    jo.end_object();
}

template<typename F>
void emit_object( JsonOut &jo, const std::string &key, F func )
{
    emit_key( jo, key );
    emit_object( jo, func );
}

/**
 * ============= HIGH-LEVEL FUNCTIONS =============
 */

void emit_file_contents( JsonOut &jo, const editor::me_file &file )
{
    emit( jo, "type", "mapgen" );
    emit( jo, "method", "json" );

    if( file.mtype == editor::MapgenType::Oter ) {
        emit( jo, "om_terrain", "field_oter" ); // TODO
        emit( jo, "weight", 100 ); // TODO
    } else if( file.mtype == editor::MapgenType::Nested ) {
        emit( jo, "nested_mapgen_id", "field_nested" ); // TODO
    } else { // editor::MapgenType::Update
        emit( jo, "update_mapgen_id", "field_update" ); // TODO
    }

    emit_object( jo, "object", [&]() {

        if( file.mtype == editor::MapgenType::Oter ) {
            if( file.oter.mapgen_base == editor::OterMapgenBase::FillTer ) {
                emit( jo, "fill_ter", file.oter.fill_ter );
            } else if( file.oter.mapgen_base == editor::OterMapgenBase::PredecessorMapgen ) {
                emit( jo, "predecessor_mapgen", file.oter.predecessor_mapgen );
            }
            if( file.oter.rotation ) {
                emit( jo, "rotation", file.oter.rotation );
            }
        } else if( file.mtype == editor::MapgenType::Nested ) {
            emit_array( jo, "mapgensize", [&]() {
                emit_val( jo, file.nested.size.x );
                emit_val( jo, file.nested.size.y );
            } );
            if( file.nested.rotation ) {
                emit( jo, "rotation", file.nested.rotation );
            }
        } else { // editor::MapgenType::Update
            if( !file.update.fill_ter.is_null() ) {
                emit( jo, "fill_ter", file.update.fill_ter );
            }
        }

        if( file.uses_rows() ) {
            emit_array( jo, "rows", [&]() {
                for( int y = 0; y < file.mapgensize().y(); y++ ) {
                    std::string s;
                    for( int x = 0; x < file.mapgensize().x(); x++ ) {
                        const map_key &mk = file.base.get_key_at( point( x, y ) );
                        s += mk.str;
                    }
                    emit_val( jo, s );
                }
            } );

            emit_object( jo, "terrain", [&]() {
                for( const editor::me_palette_entry &it : file.base.inline_palette.entries ) {
                    if( !it.ter.is_null() ) {
                        emit( jo, it.key.str, it.ter );
                    }
                }
            } );

            emit_object( jo, "furniture", [&]() {
                for( const editor::me_palette_entry &it : file.base.inline_palette.entries ) {
                    if( !it.furn.is_null() ) {
                        emit( jo, it.key.str, it.furn );
                    }
                }
            } );
        }
    } );
}

std::string to_string( const editor::me_file &file )
{
    return serialize_wrapper( [&]( JsonOut & jo ) {
        // Wrap it as 1-element array to adhere to modern BN data format
        emit_array( jo, [&]() {
            emit_object( jo, [&]() {
                emit_file_contents( jo, file );
            } );
        } );
    } );
}

std::string format_string( const std::string &js )
{
    std::stringstream in;
    std::stringstream out;

    in << js;

    JsonOut jsout( out, true );
    JsonIn jsin( in );

    format( jsin, jsout );

    out << std::endl;

    return out.str();
}

} // namespace editor_export
