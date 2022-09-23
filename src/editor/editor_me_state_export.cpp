#include "editor_me_state_export.h"

#include "../fstream_utils.h"
#include "../json.h"
#include "../../tools/format/format.h"

#include <sstream>

namespace editor_export
{

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

void emit_file_contents( JsonOut &jo, const editor::me_file &file )
{
    emit( jo, "type", "mapgen" );
    emit( jo, "method", "json" );
    emit( jo, "om_terrain", "field" ); // TODO
    emit( jo, "weight", 100 ); // TODO

    emit_object( jo, "object", [&]() {

        if( file.mtype == editor::MapgenType::Oter ) {
            emit( jo, "fill_ter", file.oter.fill_ter );
        }

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
