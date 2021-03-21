#pragma once
#ifndef CATA_SRC_BODYPART_TYPE_H
#define CATA_SRC_BODYPART_TYPE_H

#include <array>
#include <string>

#include "bodypart.h"
#include "translations.h"

struct body_part_type {
    public:
        bodypart_str_id id;
        bool was_loaded = false;

        // Those are stored untranslated
        translation name;
        translation name_multiple;
        translation accusative;
        translation accusative_multiple;
        translation name_as_heading;
        translation name_as_heading_multiple;
        std::string hp_bar_ui_text;
        std::string encumb_text;
        // Legacy "string id"
        std::string legacy_id = "num_bp";
        // Legacy enum "int id"
        body_part token = num_bp;
        /** Size of the body part when doing an unweighted selection. */
        float hit_size = 0.0f;
        /** Hit sizes for attackers who are smaller, equal in size, and bigger. */
        std::array<float, 3> hit_size_relative = {{ 0.0f, 0.0f, 0.0f }};
        /**
         * How hard is it to hit a given body part, assuming "owner" is hit.
         * Higher number means good hits will veer towards this part,
         * lower means this part is unlikely to be hit by inaccurate attacks.
         * Formula is `chance *= pow(hit_roll, hit_difficulty)`
         */
        float hit_difficulty = 0.0f;
        // "Parent" of this part - main parts are their own "parents"
        // TODO: Connect head and limbs to torso
        bodypart_str_id main_part;
        // A part that has no opposite is its own opposite (that's pretty Zen)
        bodypart_str_id opposite_part;
        // Parts with no opposites have BOTH here
        side part_side = side::BOTH;

        //Morale parameters
        float hot_morale_mod = 0;
        float cold_morale_mod = 0;

        float stylish_bonus = 0;

        int squeamish_penalty = 0;

        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;

        static void load_bp( const JsonObject &jo, const std::string &src );

        // Clears all bps
        static void reset();
        // Post-load finalization
        static void finalize_all();
        // Verifies that body parts make sense
        static void check_consistency();

        int bionic_slots() const {
            return bionic_slots_;
        }
    private:
        int bionic_slots_ = 0;
};

#endif // CATA_SRC_BODYPART_TYPE_H
