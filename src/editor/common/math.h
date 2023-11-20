#ifndef CATA_SRC_EDITOR_MATH_H
#define CATA_SRC_EDITOR_MATH_H

#include "point.h"

#include <vector>

inline int divide_wrapping( int v, int m )
{
    if( v >= 0 ) {
        return v / m;
    }
    return ( v - m + 1 ) / m;
}

inline int divide_wrapping( int v, int m, int &r )
{
    const int result = divide_wrapping( v, m );
    r = v - result * m;
    return result;
}

inline int wrap_index( int idx, int size )
{
    int ret = idx;
    divide_wrapping( idx, size, ret );
    return ret;
}

std::vector<point> line_bresenham( point a, point b );

/**
 * Given 2 opposite rectangle corners, returns lower-left and upper-right corners (min coords & max coords)
 */
template<typename Point>
std::pair<Point, Point> normalize_rect( Point a, Point b )
{
    return {
        Point( std::min( a.x(), b.x() ), std::min( a.y(), b.y() ) ),
        Point( std::max( a.x(), b.x() ), std::max( a.y(), b.y() ) )
    };
}

#endif // CATA_SRC_EDITOR_MATH_H
