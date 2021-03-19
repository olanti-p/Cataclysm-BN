#pragma once
#ifndef CATA_SRC_ITEM_CRAFT_DATA_H
#define CATA_SRC_ITEM_CRAFT_DATA_H

#include <vector>
#include <string>

#include "craft_command.h"

class JsonIn;
class JsonOut;
class JsonObject;
class recipe;

/**
 * Data for items that represent in-progress crafts.
 */
class item_craft_data
{
    public:
        const recipe *making = nullptr;
        int next_failure_point = -1;
        std::vector<item_comp> comps_used;
        // If the crafter has insufficient tools to continue to the next 5% progress step
        bool tools_to_continue = false;
        std::vector<comp_selection<tool_comp>> cached_tool_selections;

        void serialize( JsonOut &jsout ) const;
        void deserialize( JsonIn &jsin );
        void deserialize( const JsonObject &obj );
};


#endif // CATA_SRC_ITEM_CRAFT_DATA_H
