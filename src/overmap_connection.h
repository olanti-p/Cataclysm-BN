#pragma once
#ifndef CATA_SRC_OVERMAP_CONNECTION_H
#define CATA_SRC_OVERMAP_CONNECTION_H

#include <list>
#include <vector>
#include <set>
#include <string>

#include "omdata.h"
#include "type_id.h"

class JsonObject;
class JsonIn;
struct overmap_location;

class omcp_location
{
    public:
        tripoint pos;
        overmap_location_id loc;
};

class omcp_placement
{
    public:
        int basic_cost = 0;
        std::vector<omcp_location> locations;
};

class omcp_connection_exit
{
    public:
        tripoint pos;
        om_direction::type dir = om_direction::type::invalid;
        std::string conn_type;
};

class omcp_connection
{
    public:
        std::vector<omcp_connection_exit> exits;
};

class omcp_terrain
{
    public:
        tripoint pos;
        oter_str_id terrain;
};

class om_connection_piece
{
    public:
        string_id<om_connection_piece> id;
        bool was_loaded = false;

        bool is_linear = false;
        int piece_cost = 0;

        oter_type_str_id linear_terrain;
        std::string linear_conn_type;

        std::vector<omcp_placement> placements;
        std::vector<omcp_terrain> terrains;
        std::vector<omcp_connection> connections;

        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();
};

class overmap_connection
{
    public:
        class subtype
        {
                friend overmap_connection;

            public:
                enum class flag { orthogonal };

            public:
                oter_type_str_id terrain;

                int basic_cost = 0;

                bool allows_terrain( const oter_id &oter ) const;
                bool allows_turns() const {
                    return terrain->is_linear();
                }

                bool is_orthogonal() const {
                    return flags.count( flag::orthogonal );
                }

                void load( const JsonObject &jo );
                void deserialize( JsonIn &jsin );

            private:
                std::set<overmap_location_id> locations;
                std::set<flag> flags;
        };

    public:
        const subtype *pick_subtype_for( const oter_id &ground ) const;
        bool has( const oter_id &oter ) const;

        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();

    public:
        overmap_connection_id id;
        bool was_loaded = false;

        oter_type_str_id default_terrain;

        std::vector<string_id<om_connection_piece>> pieces;

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
void load_piece( const JsonObject &jo, const std::string &src );
void finalize();
void check_consistency();
void reset();

overmap_connection_id guess_for( const oter_type_id &oter );
overmap_connection_id guess_for( const oter_id &oter );

} // namespace overmap_connections

#endif // CATA_SRC_OVERMAP_CONNECTION_H
