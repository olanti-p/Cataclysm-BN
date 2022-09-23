#include "editor_me_editable_id.h"

#include "../field_type.h"
#include "../omdata.h"
#include "../mapdata.h"
#include "../mapgen.h"

namespace editor
{

template<>
const std::vector<std::string> &editable_id<field_type>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( field_types::get_all().size() );
        for( const field_type &it : field_types::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<furn_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( furn_t::get_all().size() );
        for( const furn_t &it : furn_t::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<oter_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( overmap_terrains::get_all().size() );
        for( const oter_t &it : overmap_terrains::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<mapgen_palette>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( mapgen_palette::get_all().size() );
        for( const auto &it : mapgen_palette::get_all() ) {
            all_opts.push_back( it.first.str() );
        }
    }
    return all_opts;
}

template<>
const std::vector<std::string> &editable_id<ter_t>::get_all_opts()
{
    if( all_opts.empty() ) {
        all_opts.reserve( ter_t::get_all().size() );
        for( const ter_t &it : ter_t::get_all() ) {
            all_opts.push_back( it.id.str() );
        }
    }
    return all_opts;
}

} // namespace editor
