#include "math.h"

std::vector<point> line_bresenham( point a, point b )
{
    std::vector<point> ret;
    int x0 = a.x;
    int y0 = a.y;
    int x1 = b.x;
    int y1 = b.y;
    int dx = std::abs( x1 - x0 );
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs( y1 - y0 );
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while( true ) {
        ret.emplace_back( x0, y0 );
        if( x0 == x1 && y0 == y1 ) {
            break;
        }
        int e2 = 2 * error;
        if( e2 >= dy ) {
            if( x0 == x1 ) {
                break;
            }
            error = error + dy;
            x0 = x0 + sx;
        }
        if( e2 <= dx ) {
            if( y0 == y1 ) {
                break;
            }
            error = error + dx;
            y0 = y0 + sy;
        }
    }

    return ret;
}
