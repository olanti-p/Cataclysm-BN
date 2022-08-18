#pragma once
#ifndef CATA_SRC_OM_LINES_H
#define CATA_SRC_OM_LINES_H

#include <array>
#include <cstdint>
#include <string>

#include "enums.h"
#include "om_direction.h"
#include "output.h"

namespace om_lines
{

struct type {
    uint32_t symbol;
    size_t mapgen;
    MULTITILE_TYPE subtile;
    int rotation;
    std::string suffix;
};

const std::array<std::string, 5> mapgen_suffixes = {{
        "_straight", "_curved", "_end", "_tee", "_four_way"
    }
};

const std::array < type, 1 + om_direction::bits > all = {{
        { UTF8_getch( LINE_XXXX_S ), 4, unconnected,  0, "_isolated"  }, // 0  ----
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    2, "_end_south" }, // 1  ---n
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    1, "_end_west"  }, // 2  --e-
        { UTF8_getch( LINE_XXOO_S ), 1, corner,       1, "_ne"        }, // 3  --en
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    0, "_end_north" }, // 4  -s--
        { UTF8_getch( LINE_XOXO_S ), 0, edge,         0, "_ns"        }, // 5  -s-n
        { UTF8_getch( LINE_OXXO_S ), 1, corner,       0, "_es"        }, // 6  -se-
        { UTF8_getch( LINE_XXXO_S ), 3, t_connection, 1, "_nes"       }, // 7  -sen
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    3, "_end_east"  }, // 8  w---
        { UTF8_getch( LINE_XOOX_S ), 1, corner,       2, "_wn"        }, // 9  w--n
        { UTF8_getch( LINE_OXOX_S ), 0, edge,         1, "_ew"        }, // 10 w-e-
        { UTF8_getch( LINE_XXOX_S ), 3, t_connection, 2, "_new"       }, // 11 w-en
        { UTF8_getch( LINE_OOXX_S ), 1, corner,       3, "_sw"        }, // 12 ws--
        { UTF8_getch( LINE_XOXX_S ), 3, t_connection, 3, "_nsw"       }, // 13 ws-n
        { UTF8_getch( LINE_OXXX_S ), 3, t_connection, 0, "_esw"       }, // 14 wse-
        { UTF8_getch( LINE_XXXX_S ), 4, center,       0, "_nesw"      }  // 15 wsen
    }
};

const size_t size = all.size();

constexpr size_t rotate( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    // Bitwise rotation to the left.
    return ( ( ( line << static_cast<size_t>( dir ) ) |
               ( line >> ( om_direction::size - static_cast<size_t>( dir ) ) ) ) & om_direction::bits );
}

constexpr size_t set_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    return line | 1 << static_cast<size_t>( dir );
}

constexpr bool has_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return false;
    }
    return static_cast<bool>( line & 1 << static_cast<size_t>( dir ) );
}

constexpr bool is_straight( size_t line )
{
    return line == 1
           || line == 2
           || line == 4
           || line == 5
           || line == 8
           || line == 10;
}

size_t from_dir( om_direction::type dir );

} // namespace om_lines

#endif // CATA_SRC_OM_LINES_H
