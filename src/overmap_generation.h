#pragma once
#ifndef CATA_SRC_OVERMAP_GENERATION_H
#define CATA_SRC_OVERMAP_GENERATION_H

#include <vector>

#include "coordinates.h"
#include "om_direction.h"
#include "type_id.h"

class overmap;
class overmap_connection;
class om_connection_piece;

namespace overmap_generation
{

struct ConnNode {
    // Piece position
    tripoint_om_omt pos;

    // For linear pieces:
    om_direction::type from = om_direction::type::invalid;

    // For non-linear pieces:
    string_id<om_connection_piece> piece;
    om_direction::type rot = om_direction::type::invalid;
    int conn_idx = -1;

    ConnNode() = default;
    ~ConnNode() = default;
    explicit ConnNode( const tripoint_om_omt &pos ) : pos( pos ) {}
};

struct ConnPath {
    std::vector<ConnNode> nodes;
};

ConnPath lay_out_connection(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &source,
    om_direction::type source_dir,
    const tripoint_om_omt &dest,
    om_direction::type dest_dir,
    bool must_be_unexplored
);

ConnPath lay_out_street(
    const overmap &om,
    const overmap_connection &connection,
    const tripoint_om_omt &from,
    om_direction::type dir,
    int len
);

void build_connection(
    overmap &om,
    const overmap_connection &connection,
    const ConnPath &path
);

} // namespace overmap_generation

#endif // CATA_SRC_OVERMAP_GENERATION_H
