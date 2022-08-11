#pragma once
#ifndef CATA_SRC_MAPGEN_FACTORY_H
#define CATA_SRC_MAPGEN_FACTORY_H

#include "mapgen.h"

class mapgen_basic_container
{
    public:
        std::vector<std::shared_ptr<mapgen_function>> mapgens_;
        weighted_int_list<std::shared_ptr<mapgen_function>> weights_;

        int add( const std::shared_ptr<mapgen_function> ptr );

        /**
         * Pick a mapgen function randomly and call its generate function.
         * This basically runs the mapgen functions with the given @ref mapgendata
         * as argument.
         * @return Whether the mapgen function has been run. It may not get run if
         * the list of mapgen functions is effectively empty.
         * @p hardcoded_weight Weight for an additional entry. If that entry is chosen,
         * false is returned. If unsure, just use 0 for it.
         */
        bool generate( mapgendata &dat, const int hardcoded_weight ) const;

        /**
         * Calls @ref mapgen_function::setup and sets up the internal weighted list using
         * the **current** value of @ref mapgen_function::weight. This value may have
         * changed since it was first added, so this is needed to recalculate the weighted list.
         */
        void setup();

        void check_consistency( const std::string &key );
};

class mapgen_factory
{
    public:
        std::map<std::string, mapgen_basic_container> mapgens_;

        /// Collect all the possible and expected keys that may get used with @ref pick.
        static std::set<std::string> get_usages();

        void reset();

        /// @see mapgen_basic_container::setup
        void setup();

        void check_consistency();

        /**
         * Checks whether we have an entry for the given key.
         * Note that the entry itself may not contain any valid mapgen instance
         * (could all have been removed via @ref erase).
         */
        bool has( const std::string &key ) const;

        /// @see mapgen_basic_container::add
        int add( const std::string &key, const std::shared_ptr<mapgen_function> ptr );

        /// @see mapgen_basic_container::generate
        bool generate( mapgendata &dat, const std::string &key, const int hardcoded_weight = 0 ) const;
};

#endif // CATA_SRC_MAPGEN_FACTORY_H
