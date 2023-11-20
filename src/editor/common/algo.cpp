#include "algo.h"

namespace editor
{

std::vector<point> find_tiles_via_global( const Canvas2D<UUID> &canvas,
        std::function<bool( point p, const UUID & )> predicate )
{
    std::vector<point> ret;

    for( int x = 0; x < canvas.get_size().x; x++ ) {
        for( int y = 0; y < canvas.get_size().y; y++ ) {
            point p( x, y );
            const UUID &t = canvas.get( p );
            if( predicate( p, t ) ) {
                ret.push_back( p );
            }
        }
    }

    return ret;
}

std::vector<point> find_tiles_via_floodfill( const Canvas2D<UUID> &canvas,
        const point &initial_pos,
        std::function<bool( point p, const UUID & )> predicate )
{
    std::vector<point> ret;

    if( !predicate( initial_pos, canvas.get( initial_pos ) ) ) {
        return ret;
    }

    std::set<point> open;
    std::set<point> closed;
    open.insert( initial_pos );

    while( !open.empty() ) {
        auto it = open.cbegin();
        point p = *it;
        open.erase( it );
        closed.insert( p );
        ret.push_back( p );
        for( const point &d : neighborhood ) {
            point p2 = p + d;
            if( p2.x < 0 || p2.y < 0 || p2.x >= canvas.get_size().x || p2.y >= canvas.get_size().y ) {
                continue;
            }
            if( closed.count( p2 ) != 0 ) {
                continue;
            }
            closed.insert( p2 );
            if( predicate( p2, canvas.get( p2 ) ) ) {
                open.insert( p2 );
            }
        }
    }

    return ret;
}

} // namespace editor
