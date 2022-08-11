#include "mapgen_factory.h"

#include "debug.h"
#include "generic_factory.h"
#include "input.h"
#include "map_extras.h"
#include "map.h"
#include "omdata.h"
#include "rng.h"

int mapgen_basic_container::add( const std::shared_ptr<mapgen_function> ptr )
{
    assert( ptr );
    if( std::find( mapgens_.begin(), mapgens_.end(), ptr ) != mapgens_.end() ) {
        debugmsg( "Adding duplicate mapgen to container!" );
    }
    mapgens_.push_back( ptr );
    return mapgens_.size() - 1;
}

bool mapgen_basic_container::generate( mapgendata &dat, const int hardcoded_weight ) const
{
    if( hardcoded_weight > 0 &&
        rng( 1, weights_.get_weight() + hardcoded_weight ) > weights_.get_weight() ) {
        return false;
    }
    const std::shared_ptr<mapgen_function> *const ptr = weights_.pick();
    if( !ptr ) {
        return false;
    }
    assert( *ptr );
    ( *ptr )->generate( dat );
    return true;
}

void mapgen_basic_container::setup()
{
    for( const std::shared_ptr<mapgen_function> &ptr : mapgens_ ) {
        const int weight = ptr->weight;
        if( weight < 1 ) {
            continue; // rejected!
        }
        weights_.add( ptr, weight );
        ptr->setup();
    }
    // Not needed anymore, pointers are now stored in weights_ (or not used at all)
    mapgens_.clear();
}

void mapgen_basic_container::check_consistency( const std::string &key )
{
    for( auto &mapgen_function_ptr : weights_ ) {
        mapgen_function_ptr.obj->check( key );
    }
}

std::set<std::string> mapgen_factory::get_usages()
{
    std::set<std::string> result;
    for( const oter_t &elem : overmap_terrains::get_all() ) {
        result.insert( elem.get_mapgen_id() );
        result.insert( elem.id.str() );
    }
    // Why do I have to repeat the MapExtras here? Wouldn't "MapExtras::factory" be enough?
    for( const map_extra &elem : MapExtras::mapExtraFactory().get_all() ) {
        if( elem.generator_method == map_extra_method::mapgen ) {
            result.insert( elem.generator_id );
        }
    }
    // Used in C++ code only, see calls to `oter_mapgen.generate()` below
    result.insert( "lab_1side" );
    result.insert( "lab_4side" );
    result.insert( "lab_finale_1level" );
    return result;
}

void mapgen_factory::reset()
{
    mapgens_.clear();
}

void mapgen_factory::setup()
{
    for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
        omw.second.setup();
        inp_mngr.pump_events();
    }
    // Dummy entry, overmap terrain null should never appear and is therefor never generated.
    mapgens_.erase( "null" );
}

void mapgen_factory::check_consistency()
{
    // Cache all strings that may get looked up here so we don't have to go through
    // all the sources for them upon each loop.
    const std::set<std::string> usages = get_usages();
    for( std::pair<const std::string, mapgen_basic_container> &omw : mapgens_ ) {
        omw.second.check_consistency( omw.first );
        if( usages.count( omw.first ) == 0 ) {
            debugmsg( "Mapgen %s is not used by anything!", omw.first );
        }
    }
}

bool mapgen_factory::has( const std::string &key ) const
{
    return mapgens_.count( key ) != 0;
}

int mapgen_factory::add( const std::string &key, const std::shared_ptr<mapgen_function> ptr )
{
    return mapgens_[key].add( ptr );
}

bool mapgen_factory::generate( mapgendata &dat, const std::string &key,
                               const int hardcoded_weight ) const
{
    const auto iter = mapgens_.find( disable_mapgen ? "test" : key );
    if( iter == mapgens_.end() ) {
        return false;
    }
    return iter->second.generate( dat, hardcoded_weight );
}
