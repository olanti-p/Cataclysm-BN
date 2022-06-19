#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <functional>
#include <vector>
#include <utility>

#include "coordinates.h"
#include "enums.h"
#include "hash_utils.h"
#include "omdata.h"
#include "optional.h"
#include "point.h"

namespace pf
{

/*
 * A node in a path, containing position and direction.
 */
template<typename Point>
struct directed_node {
    Point pos;
    om_direction::type dir;

    explicit directed_node( Point pos,
                            om_direction::type dir = om_direction::type::invalid ) : pos( pos ), dir( dir ) {}
};

template<typename Point>
struct directed_node_alt {
    Point pos;
    int var = -1;
    int conn = -1;
    om_direction::type rot = om_direction::type::invalid;

    directed_node_alt() = default;
    ~directed_node_alt() = default;
    explicit directed_node_alt( Point pos, int var, om_direction::type rot, int conn ) :
        pos( pos ), var( var ), conn( conn ), rot( rot ) {}

    template<typename P>
    explicit directed_node_alt( Point pos, const directed_node_alt<P> &rhs ) :
        pos( pos ), var( rhs.var ), conn( rhs.conn ), rot( rhs.rot ) {}

    constexpr inline bool operator==( const directed_node_alt &rhs ) const {
        return pos == rhs.pos && var == rhs.var && rot == rhs.rot && conn == rhs.conn;
    }
};

/*
 * Data structure representing a path from a source to a destination.
 * The nodes are given in reverse order (from destination to source) in order to allow
 * efficient consumption through pop_back().
 */
template<typename Point>
struct directed_path {
    std::vector<directed_node<Point>> nodes;
};

template<typename Point>
struct directed_path_alt {
    std::vector<directed_node_alt<Point>> nodes;
};

/*
 * Data structure representing a path from a source to a destination.
 * The points are given in reverse order (from destination to source) in order to allow
 * efficient consumption through pop_back().
 */
template<typename Point>
struct simple_path {
    std::vector<Point> points;
};

// Data structure returned by a node scoring function.
struct node_score {
    // cost of traversing the "current" node
    // value < 0 means it can not be traversed
    int node_cost;
    // estimated cost to reach the destination from the "current" node
    // if node_cost is negative, this is ignored
    int estimated_dest_cost;

    node_score( int node_cost, int estimated_dest_cost );

    static const node_score rejected;
};

// A node scoring function that provides a node to score and optionally provides the
// previous node in the path as context.
template<typename Point>
using two_node_scoring_fn =
    std::function<node_score( directed_node<Point>, cata::optional<directed_node<Point>> )>;

// non-templated implementation
directed_path<point> greedy_path( const point &source, const point &dest, const point &max,
                                  two_node_scoring_fn<point> scorer );

template<typename Point>
using neighbor_provider_cb = std::function<void( const directed_node_alt<Point>&, float )>;

template<typename Point>
using neighbor_provider =
    std::function<void( const directed_node_alt<Point>& cur, neighbor_provider_cb<Point> cb )>;

template<typename Node>
class PathFinder
{
    private:

        Node start;
        Node current;
        std::unordered_map<Node, Node> came_from;
        std::unordered_map<Node, float> g_scores;
        std::unordered_map<Node, float> f_scores;
        std::vector<Node> open_set;

        size_t find_node_with_lowest_f() {
            size_t min_i = 0;
            float min_f = std::numeric_limits<float>::infinity();
            for( size_t i = 0; i < open_set.size(); i++ ) {
                auto it = f_scores.find( open_set[i] );
                if( it != f_scores.end() ) {
                    float f_score = it->second;
                    if( f_score < min_f ) {
                        min_i = i;
                        min_f = f_score;
                    }
                }
            }
            return min_i;
        }

        std::vector<Node> reconstruct_path() {
            std::vector<Node> total_path;
            Node curr = current;
            for( ;; ) {
                total_path.insert( total_path.begin(), curr );
                auto it = came_from.find( curr );
                if( it == came_from.end() ) {
                    break;
                }
                curr = it->second;
            } // for(;;)
            return total_path;
        }

        template<typename FuncConflicts>
        bool check_conflict_with_recent( const Node &current, const Node &target, int steps,
                                         FuncConflicts conflict_check_func ) {
            const Node *cur = &current;
            for( int step = 0; step < steps; step++ ) {
                auto it = came_from.find( *cur );
                if( it == came_from.end() ) {
                    break;
                }
                cur = &it->second;
                if( conflict_check_func( *cur, target ) ) {
                    return true;
                }
            }
            return false;
        }

    public:
        PathFinder() = default;
        ~PathFinder() = default;

        template<typename FuncH, typename FuncNeighbors, typename FuncGoalCheck, typename FuncConflicts>
        std::vector<Node> find_path( Node p_start,
                                     FuncH h_func,
                                     FuncNeighbors neighbor_provider,
                                     FuncGoalCheck goal_check_func,
                                     FuncConflicts conflict_check_func ) {
            start = p_start;

            open_set.reserve( 32 );
            open_set.push_back( start );

            g_scores[start] = 0;
            f_scores[start] = h_func( start );

            int num_iterations = 300000;

            while( !open_set.empty() ) {
                size_t current_i = find_node_with_lowest_f();
                current = open_set[current_i];

                if( goal_check_func( current ) ) {
                    return reconstruct_path();
                }

                num_iterations -= 1;
                if( num_iterations == 0 ) {
                    break;
                }

                open_set.erase( open_set.begin() + current_i );

                float current_g_score;
                {
                    auto it = g_scores.find( current );
                    if( it == g_scores.end() ) {
                        current_g_score = std::numeric_limits<float>::infinity();
                    } else {
                        current_g_score = it->second;
                    }
                }

                neighbor_provider( current, [&]( const Node & neighbor, float d_score ) {
                    if( check_conflict_with_recent( current, neighbor, 40, conflict_check_func ) ) {
                        // Intersects path
                        return;
                    }
                    float neighbor_g_score;
                    {
                        auto it = g_scores.find( neighbor );
                        if( it == g_scores.end() ) {
                            neighbor_g_score = std::numeric_limits<float>::infinity();
                        } else {
                            neighbor_g_score = it->second;
                        }
                    }
                    float tentative_g_score = current_g_score + d_score;
                    if( tentative_g_score < neighbor_g_score ) {
                        float h_score = h_func( neighbor );
                        came_from[neighbor] = current;
                        g_scores[neighbor] = tentative_g_score;
                        f_scores[neighbor] = tentative_g_score + h_score;
                        {
                            auto it = std::find( open_set.begin(), open_set.end(), neighbor );
                            if( it == open_set.end() ) {
                                open_set.push_back( neighbor );
                            }
                        }
                    }
                } );
            }

            return {};
        }
};

/**
 * Uses Greedy Best-First-Search to find a short path from source to destination [2D only].
 * The search area is a rectangle with corners at (0,0) and max.
 *
 * @param source Starting point of path
 * @param dest End point of path
 * @param max Max permissible coordinates for a point on the path
 * @param scorer function of (node &current, node *previous) that returns node_score.
 */
template<typename Point, typename = std::enable_if_t<Point::dimension == 2>>
directed_path<Point> greedy_path( const Point &source, const Point &dest, const Point &max,
                                  two_node_scoring_fn<Point> scorer )
{
    directed_path<Point> res;
    const two_node_scoring_fn<point> point_scorer
    = [scorer]( directed_node<point> current, cata::optional<directed_node<point>> prev ) {
        cata::optional<directed_node<Point>> prev_node;
        if( prev ) {
            prev_node = directed_node<Point>( Point( prev->pos ), prev->dir );
        }
        return scorer( directed_node<Point>( Point( current.pos ), current.dir ), prev_node );
    };
    directed_path<point> path = greedy_path( source.raw(), dest.raw(), max.raw(), point_scorer );
    res.nodes.reserve( path.nodes.size() );
    for( const auto &node : path.nodes ) {
        res.nodes.emplace_back( Point( node.pos ), node.dir );
    }
    return res;
}

struct omt_score {
    // cost of traversing the "current" OMT
    // value < 0 means it can not be traversed
    int node_cost;
    // Set to true if it *may* be possible to go up/down from the "current" node.
    // The relevant scoring function will be invoked to determine if it is
    // actually possible.
    bool allow_z_change;

    explicit omt_score( int node_cost, bool allow_z_change = false );

    static const omt_score rejected;
};

using omt_scoring_fn = std::function<omt_score( tripoint_abs_omt )>;

/**
 * Uses A* to find an approximately-cheapest path from source to destination (in 3D).
 *
 * @param source Starting point of path
 * @param dest End point of path
 * @param radius Maximum search radius
 * @param scorer function that returns the omt_score for the given OMT
 * @param max_cost Maximum path cost (optional)
 */
simple_path<tripoint_abs_omt> find_overmap_path( const tripoint_abs_omt &source,
        const tripoint_abs_omt &dest, int radius, omt_scoring_fn scorer,
        cata::optional<int> max_cost = cata::nullopt );

} // namespace pf

namespace std
{
template <typename Point>
struct hash<pf::directed_node_alt<Point>> {
    std::size_t operator()( const pf::directed_node_alt<Point> &k ) const noexcept {
        size_t seed = 0x9e3779b9;
        cata::hash_combine( seed, k.pos );
        cata::hash_combine( seed, k.var );
        cata::hash_combine( seed, static_cast<int>( k.rot ) );
        return seed;
    }
};
} // namespace std

#endif // CATA_SRC_SIMPLE_PATHFINDING_H
