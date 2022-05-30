#pragma once
#ifndef CATA_SRC_OVERMAP_CONNECTION_H
#define CATA_SRC_OVERMAP_CONNECTION_H

#include <array>
#include <list>
#include <vector>
#include <set>
#include <string>

#include "int_id.h"
#include "omdata.h"
#include "string_id.h"

class JsonObject;
class JsonIn;
struct overmap_location;

using overmap_location_str_id = string_id<overmap_location>;

struct om_conn_upgrade {
        friend struct om_connection_new;
    public:
        int segment = -1;
        om_direction::type rot = om_direction::type::invalid;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );

    private:
        oter_type_str_id segment_str;
};

struct om_conn_segment {
        friend struct om_connection_new;
    public:
        oter_type_str_id terrain;
        float complexity_cost = 0.0f;
        std::vector<om_conn_upgrade> upgrades;
        int rotates = 1;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );

        const std::vector<int> &get_edge_of_rotated(
            om_direction::type side,
            om_direction::type rot,
            int conn_id
        ) const;

        inline int get_num_connections() const {
            return static_cast<int>( connections.size() );
        }

    private:
        inline std::vector<std::string> &get_edge_mut( om_direction::type side ) {
            return edges[static_cast<int>( side )];
        }

        std::array<std::vector<std::string>, 4> edges;
        std::vector<std::string> connections_str;
        std::vector<std::array<std::vector<int>, 4>> connections;
};

struct om_conn_location {
        friend struct om_connection_new;
    public:
        overmap_location_str_id id;
        float basic_cost = 0.0f;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );
};

struct om_conn_placement {
        friend struct om_connection_new;
    public:
        std::vector<om_conn_location> locations;
        std::vector<int> segments;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );

    private:
        std::vector<oter_type_str_id> segments_str;
};

struct om_connection_new {
    public:
        string_id<overmap_connection> id;

        int default_segment = -1;
        float follow_cost = 0.0f;

        std::vector<om_conn_segment> segments;
        std::vector<om_conn_placement> placements;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );
        void check() const;
        void finalize();

        int find_segment_by_terr( const oter_type_str_id &seg ) const;
        const std::vector<int> &find_candidate_segments( const oter_id &t ) const;
        float get_terrain_cost( const oter_id &t ) const;

    private:
        oter_type_str_id default_segment_str;
        std::unordered_map<std::string, int> edge_string_hash;
};

bool test_segment_connectivity(
    const std::vector<int> &edge_src,
    const std::vector<int> &edge_dest
);

class overmap_connection
{
    public:
        class subtype
        {
                friend overmap_connection;

            public:
                enum class flag { orthogonal };

            public:
                string_id<oter_type_t> terrain;

                int basic_cost = 0;

                bool allows_terrain( const int_id<oter_t> &oter ) const;
                bool allows_turns() const {
                    return terrain->is_linear();
                }

                bool is_orthogonal() const {
                    return flags.count( flag::orthogonal );
                }

                void load( const JsonObject &jo );
                void deserialize( JsonIn &jsin );

            private:
                std::set<string_id<overmap_location>> locations;
                std::set<flag> flags;
        };

    public:
        const subtype *pick_subtype_for( const int_id<oter_t> &ground ) const;
        bool has( const int_id<oter_t> &oter ) const;

        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();

    public:
        string_id<overmap_connection> id;
        bool was_loaded = false;
        bool use_new_method = false;
        bool disable_city_hubs = false;
        om_connection_new data_new;

    private:
        struct cache {
            const subtype *value = nullptr;
            bool assigned = false;
            operator bool() const {
                return assigned;
            }
        };

        std::list<subtype> subtypes;
        mutable std::vector<cache> cached_subtypes;
};

namespace overmap_connections
{

void load( const JsonObject &jo, const std::string &src );
void finalize();
void check_consistency();
void reset();
const std::vector<overmap_connection> &get_all();

string_id<overmap_connection> guess_for( const int_id<oter_type_t> &oter_id );
string_id<overmap_connection> guess_for( const int_id<oter_t> &oter_id );

} // namespace overmap_connections

#endif // CATA_SRC_OVERMAP_CONNECTION_H
