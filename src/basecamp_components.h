#pragma once
#ifndef CATA_SRC_BASECAMP_COMPONENTS_H
#define CATA_SRC_BASECAMP_COMPONENTS_H

#include <vector>
#include <string>
#include <memory>

#include "craft_command.h"

class recipe;
class basecamp;
class tinymap;

class basecamp_action_components
{
    public:
        basecamp_action_components( const recipe &making, int batch_size, basecamp & );

        // Returns true iff all necessary components were successfully chosen
        bool choose_components();
        void consume_components();
    private:
        const recipe &making_;
        int batch_size_;
        basecamp &base_;
        std::vector<comp_selection<item_comp>> item_selections_;
        std::vector<comp_selection<tool_comp>> tool_selections_;
        std::unique_ptr<tinymap> map_; // Used for by-radio crafting
};

#endif // CATA_SRC_BASECAMP_COMPONENTS_H
