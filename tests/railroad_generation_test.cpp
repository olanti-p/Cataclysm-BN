#include "catch/catch.hpp"

#include "overmap.h"
#include "point.h"

static std::unordered_map<char32_t, std::string> char_legend = {{
        { U'.', "field" },
        { U'#', "rock_border" },
        { U'^', "railroad_end_2_north" },
        { U'>', "railroad_end_2_east" },
        { U'v', "railroad_end_2_south" },
        { U'<', "railroad_end_2_west" },
        { U'│', "railroad_straight_north" },
        { U'─', "railroad_straight_east" },
        { U'┊', "railroad_straight_south" },
        { U'┈', "railroad_straight_west" },
        { U'┃', "railroad_straight_end_north" },
        { U'━', "railroad_straight_end_east" },
        { U'┋', "railroad_straight_end_south" },
        { U'┉', "railroad_straight_end_west" },
    }
};

using canvas = std::vector<std::u32string>;

static point get_canvas_size( const canvas& c ) {
    if (c.empty()) {
        return point_zero;
    } else {
        return point(
                   static_cast<int>( c[0].size() ),
                   static_cast<int>( c.size() )
               );
    }
}

static void validate_size( const canvas& c, const point& sz) {
    REQUIRE( static_cast<int>( c.size() ) == sz.y );
    for (const auto& line : c) {
        REQUIRE( static_cast<int>(line.size()) == sz.x );
    }
}

static void check_equals( const overmap& om, const point& sz, const canvas& expected) {
    for (int y = 0; y < sz.y; y++) {
        const auto& line = expected[y];
        for ( int x = 0; x < sz.x; x++) {
            CAPTURE(x);
            CAPTURE(y);
            oter_id ter = om.ter(tripoint_om_omt(x+1, y+1, 0));
            const std::string& ter_id = ter->id.str();
            const std::string& expected_id = char_legend[line[x]];
            CHECK( ter_id == expected_id );
        }
    }
}

class railroad_gen_tester {
public:
    void railroad_test(
        int test_num,
        point start,
        om_direction::type start_dir,
        point dest,
        om_direction::type dest_dir,
        const canvas& initial,
        const canvas& expected
    ) const;
};

void railroad_gen_tester::railroad_test(
    int test_num,
    point start,
    om_direction::type start_dir,
    point dest,
    om_direction::type dest_dir,
    const canvas& initial,
    const canvas& expected
) const {
    CAPTURE(test_num);

    point sz = get_canvas_size( initial );
    validate_size(initial, sz);
    validate_size(expected, sz);

    oter_str_id block_str("rock_border");
    oter_id block = block_str.id();

    overmap om(point_abs_om(0, 0));
    for (int x = 0; x < sz.x + 2; x++) {
        om.ter_set(tripoint_om_omt(x, 0, 0), block);
        om.ter_set(tripoint_om_omt(x, sz.y + 1, 0), block);
    }
    for (int y = 0; y < sz.y + 2; y++) {
        om.ter_set(tripoint_om_omt(0, y, 0), block);
        om.ter_set(tripoint_om_omt(sz.x + 1, y, 0), block);
    }

    for (int y = 0; y < sz.y; y++) {
        const auto& line = initial[y];
        for ( int x = 0; x < sz.x; x++) {
            oter_str_id this_id(char_legend[line[x]]);
            om.ter_set(tripoint_om_omt(x+1, y+1, 0), this_id.id());
        }
    }

    check_equals( om, sz, initial );

    const overmap_connection& connection = string_id<overmap_connection>("railroad_new").obj();

    om.build_connection(
        point_om_omt(start),
        point_om_omt(dest),
        0,
        connection,
        false,
        start_dir,
        dest_dir
    );

    check_equals( om, sz, expected );
}

TEST_CASE("railroad_gen", "[mapgen][connects]")
{
    static canvas empty_10_10 = {{
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U".........."
        }
    };

    railroad_gen_tester().railroad_test(
        1,
        point(1, 1),
        om_direction::type::invalid,
        point(3, 1),
        om_direction::type::invalid,
        empty_10_10,
    {{
            U"..........",
            U".>─<......",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U".........."
        }
    }
    );

    railroad_gen_tester().railroad_test(
        2,
        point(1, 1),
        om_direction::type::invalid,
        point(1, 3),
        om_direction::type::invalid,
        empty_10_10,
    {{
            U"..........",
            U".v........",
            U".│........",
            U".^........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U"..........",
            U".........."
        }
    }
    );
}
