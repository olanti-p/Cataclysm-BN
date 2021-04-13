#include "cata_libintl.h"

#include "catch/catch.hpp"
#include "filesystem.h"
#include "string_formatter.h"
#include "rng.h"

#include <array>
#include <iostream>

struct test_case_data {
    int serial;
    std::string input;
    std::string expected;
};

static const std::vector<test_case_data> tests_plural_form_rules = {{
        {
            0, // a valid expression
            "n%2",
            "(n%2)",
        },
        {
            1, // same as previous, but with brackets and spaces
            " ( n % 2 ) ",
            "(n%2)",
        },
        {
            2, // ternary op
            "n?0:1",
            "(n?0:1)",
        },
        {
            3, // two ternary ops
            "n?1?2:3:4",
            "(n?(1?2:3):4)",
        },
        {
            4, // ternary op priority
            "n==1?n%2:n%3",
            "((n==1)?(n%2):(n%3))",
        },
        {
            5, // simple op priority
            "n%10==1 && n%100!=11",
            "(((n%10)==1)&&((n%100)!=11))",
        },
        {
            6, // maximum integer
            "n == 4294967295 ? 1 : 0",
            "((n==4294967295)?1:0)",
        },
        {
            7, // English
            "n!=1",
            "(n!=1)",
        },
        {
            8, // French
            "n>1",
            "(n>1)",
        },
        {
            9, // Japanese
            "0",
            "0",
        },
        {
            10, // Latvian
            "n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2",
            "((((n%10)==1)&&((n%100)!=11))?0:((n!=0)?1:2))",
        },
        {
            11, // Polish (GNU version)
            "n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2",
            "((n==1)?0:((((n%10)>=2)&&(((n%10)<=4)&&(((n%100)<10)||((n%100)>=20))))?1:2))",
        },
        {
            12, // Russian (GNU version)
            "n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2",
            "((((n%10)==1)&&((n%100)!=11))?0:((((n%10)>=2)&&(((n%10)<=4)&&(((n%100)<10)||((n%100)>=20))))?1:2))",
        },
    }
};

static const std::vector<test_case_data> tests_plural_form_rules_fail = {{
        {
            0, // missing right-hand expression
            "n%",
            "expected expression at pos 2",
        },
        {
            1, // missing left-hand expression
            "%2",
            "expected expression at pos 0",
        },
        {
            2, // missing op
            "n2",
            "unexpected token at pos 1",
        },
        {
            3, // missing closing bracket
            " ( n % 2 ",
            "expected closing bracket at pos 9",
        },
        {
            4, // stray closing bracket
            "  n % 2     )  ",
            "unexpected token at pos 12",
        },
        {
            5, // empty expression
            "  ",
            "expected expression at pos 2",
        },
        {
            6, // missing op
            " ( n % 2 ) 2 % n",
            "unexpected token at pos 11",
        },
        {
            7, // missing right-hand expression
            " ( n % 2 ) % % 4",
            "expected expression at pos 13",
        },
        {
            8, // missing left-hand expression
            "%% 3",
            "expected expression at pos 0",
        },
        {
            9, // unknown op
            "n % -3",
            "unexpected character '-' at pos 4",
        },
        {
            10, // unknown op
            "n * 3",
            "unexpected character '*' at pos 2",
        },
        {
            11, // extra closing bracket
            "(((((n % 3))))))",
            "unexpected token at pos 15",
        },
        {
            12, // missing op
            "n % 2 3",
            "unexpected token at pos 6",
        },
        {
            13, // integer overflow
            "n == 4294967296 ? 1 : 0",
            "invalid number '4294967296' at pos 5",
        },
        {
            14, // missing ternary delimiter
            "n ? 2 3",
            "expected ternary delimiter at pos 6",
        },
    }
};

// MO Plural forms expression parsing
TEST_CASE( "mo_plurals_parsing", "[libintl][i18n]" )
{
    for( const auto &it : tests_plural_form_rules ) {
        CAPTURE( it.serial );
        cata_internal::PlfNodePtr ptr = cata_internal::parse_plural_rules( it.input );
        REQUIRE( ptr );
        CHECK( ptr->debug_dump() == it.expected );
    }
    for( const auto &it : tests_plural_form_rules_fail ) {
        CAPTURE( it.serial );
        try {
            cata_internal::PlfNodePtr ptr = cata_internal::parse_plural_rules( it.input );
            CAPTURE( ptr->debug_dump() );
            FAIL_CHECK();
        } catch( std::runtime_error err ) {
            CHECK( err.what() == it.expected );
        }
    }
}

// MO Plural forms calculation
TEST_CASE( "mo_plurals_calculation", "[libintl][i18n]" )
{
    constexpr size_t NUM_MANUAL_FORMS = 130;
    constexpr size_t PLF_PERIOD = 100;

    // Plural forms for Russian for numbers 0..129
    static const std::array<unsigned long, NUM_MANUAL_FORMS> expected_plural_values = {{
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 0..9
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 10..19
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 20..29
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 30..39
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 40..49
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 50..59
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 60..69
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 70..79
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 80..89
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 90..99
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 100..109
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 110..119
            2, 0, 1, 1, 1, 2, 2, 2, 2, 2, // 120..129
        }
    };

    // Plural rules for Russian
    std::string expr_raw =
        "n%10==1 && n%100!=11 ? 0 : n%10>1 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2";
    // Original string comes from GNU gettext documentation:
    // "n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2";
    //                                 ^^^-- this part was changed
    // The change was done so that this expression would include *every* supported operator
    cata_internal::PlfNodePtr expr = cata_internal::parse_plural_rules( expr_raw );

    SECTION( "Produces expected values for small numbers" ) {
        for( size_t i = 0; i < NUM_MANUAL_FORMS; i++ ) {
            unsigned long x = i;
            unsigned long exp = expected_plural_values[i];

            CAPTURE( x );
            unsigned long res = expr->eval( x );
            CHECK( exp == res );
        }
    };

    SECTION( "Produces expected values for big numbers" ) {
        constexpr size_t CHECK_MAX = 1'234'567;

        for( size_t i = NUM_MANUAL_FORMS; i < CHECK_MAX; i++ ) {
            unsigned long x = i;
            unsigned long exp = expected_plural_values[i % PLF_PERIOD];

            CAPTURE( x );
            unsigned long res = expr->eval( x );
            if( exp != res ) {
                REQUIRE( exp == res );
            }
        }
    };

    SECTION( "Produces expected values for any numbers" ) {
        constexpr size_t CHECK_TOTAL = 1'000'000;

        for( size_t i = 0; i < CHECK_TOTAL; i++ ) {
            unsigned long x;
            if( i == 0 ) {
                x = std::numeric_limits<unsigned long>::max();
            } else {
                static std::uniform_int_distribution<unsigned long> rng_uint_dist;
                x = rng_uint_dist( rng_get_engine() );
            }
            unsigned long exp = expected_plural_values[x % PLF_PERIOD];

            CAPTURE( x );
            unsigned long res = expr->eval( x );
            if( exp != res ) {
                REQUIRE( exp == res );
            }
        }
    };

    // For some languages Transifex defines additional plural form for fractions.
    // Neither GNU gettext nor cata_libintl support fractional numbers, so
    // the extra plural form goes unused. Relevant issue:
    // https://github.com/cataclysmbnteam/Cataclysm-BN/issues/432
    //
    // This test reaffirms the assumption that both Transifex's and GNU's plf expressions
    // produce same values for integer numbers.
    SECTION( "GNU gettext rules equal Transifex rules" ) {
        constexpr size_t CHECK_TOTAL = 1'000'000;

        struct rules {
            int serial;
            std::string gnu;
            std::string tfx;
        };

        static std::vector<rules> rules_to_compare = {{
                {
                    0, // Polish
                    "(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)",
                    "(n==1 ? 0 : (n%10>=2 && n%10<=4) && (n%100<12 || n%100>14) ? 1 : n!=1"
                    "&& (n%10>=0 && n%10<=1) || (n%10>=5 && n%10<=9) || (n%100>=12 && n%100<=14) ? 2 : 3)"
                },
                {
                    1, // Russian
                    "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)",
                    "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 :"
                    " n%10==0 || (n%10>=5 && n%10<=9) || (n%100>=11 && n%100<=14)? 2 : 3)"
                },
                {
                    2, // Ukrainian
                    "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2)",
                    "(n % 1 == 0 && n % 10 == 1 && n % 100 != "
                    "11 ? 0 : n % 1 == 0 && n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 12 || n % "
                    "100 > 14) ? 1 : n % 1 == 0 && (n % 10 ==0 || (n % 10 >=5 && n % 10 <=9) || "
                    "(n % 100 >=11 && n % 100 <=14 )) ? 2: 3)"
                }
            }
        };

        for( const rules &it : rules_to_compare ) {
            CAPTURE( it.serial );
            cata_internal::PlfNodePtr expr_gnu = cata_internal::parse_plural_rules( it.gnu );
            cata_internal::PlfNodePtr expr_tfx = cata_internal::parse_plural_rules( it.tfx );

            for( size_t i = 0; i < CHECK_TOTAL; i++ ) {
                static std::uniform_int_distribution<unsigned long> rng_uint_dist;
                unsigned long x = rng_uint_dist( rng_get_engine() );

                CAPTURE( x );

                unsigned long res_gnu = expr_gnu->eval( x );
                unsigned long res_tfx = expr_tfx->eval( x );
                if( res_gnu != res_tfx ) {
                    REQUIRE( res_gnu == res_tfx );
                }
            }
        }
    };
}

static void test_get_strings( const trans_library &lib )
{
    const auto tst = []( int serial, const char *s, const char *expected ) {
        CAPTURE( serial );
        REQUIRE( s );
        CHECK( std::string( s ) == expected );
    };

    tst( 1, lib.get( "Cataclysm" ), "Катаклизм" );

    tst( 11, lib.get_ctx( "noun", "Test" ), "Тест" );
    tst( 12, lib.get_ctx( "verb", "Test" ), "Тестировать" );

    tst( 21, lib.get_pl( "%d item", "%d items", 1 ), "%d предмет" );
    tst( 22, lib.get_pl( "%d item", "%d items", 2 ), "%d предмета" );
    tst( 23, lib.get_pl( "%d item", "%d items", 5 ), "%d предметов" );

    tst( 31, lib.get_ctx_pl( "source of water", "%d spring", "%d springs", 1 ), "%d родник" );
    tst( 32, lib.get_ctx_pl( "source of water", "%d spring", "%d springs", 2 ), "%d родника" );
    tst( 33, lib.get_ctx_pl( "source of water", "%d spring", "%d springs", 5 ), "%d родников" );
    tst( 34, lib.get_ctx_pl( "metal coil", "%d spring", "%d springs", 1 ), "%d пружина" );
    tst( 35, lib.get_ctx_pl( "metal coil", "%d spring", "%d springs", 2 ), "%d пружины" );
    tst( 36, lib.get_ctx_pl( "metal coil", "%d spring", "%d springs", 5 ), "%d пружин" );
}

static const std::string mo_dir = "tests/data/cata_libintl/";

// Load single MO and get strings
TEST_CASE( "single_mo_strings", "[libintl][i18n]" )
{
    SECTION( "Little endian file" ) {
        std::vector<trans_catalogue> list;
        list.push_back( trans_catalogue::load_from_file( mo_dir + "single_ru_little_endian.mo" ) );
        trans_library lib = trans_library::create( std::move( list ) );

        test_get_strings( lib );
    }
    SECTION( "Big endian file" ) {
        std::vector<trans_catalogue> list;
        list.push_back( trans_catalogue::load_from_file( mo_dir + "single_ru_big_endian.mo" ) );
        trans_library lib = trans_library::create( std::move( list ) );

        test_get_strings( lib );
    }
}

// Load multiple MO and get strings
TEST_CASE( "multiple_mo_strings", "[libintl][i18n]" )
{
    std::vector<trans_catalogue> list;
    list.push_back( trans_catalogue::load_from_file( mo_dir + "multi_1_ru.mo" ) );
    list.push_back( trans_catalogue::load_from_file( mo_dir + "multi_2_ru.mo" ) );
    list.push_back( trans_catalogue::load_from_file( mo_dir + "multi_3_ru.mo" ) );
    trans_library lib = trans_library::create( std::move( list ) );

    test_get_strings( lib );
}

static const std::vector<test_case_data> tests_mo_loading_failures = {{
        {
            0, // file not found
            "non-existent.mo",
            "failed to open file",
        },
        {
            1, // not a MO file (magic number mismatch)
            "single.pot",
            "not a MO file",
        },
        {
            2, // not a MO file (too small to have magic number)
            "empty_file.mo",
            "not a MO file",
        },
        {
            3, // wrong charset (only UTF-8 is supported)
            "wrong_charset_ru.mo",
            "unexpected value in Content-Type header (wrong charset?)",
        },
        {
            4, // one of the strings extends beyond end of file
            "single_ru_string_ignores_eof.mo",
            "string_info at 0x84: extends beyond EOF (len:0x16 addr:0x35f file size:0x375)",
        },
        {
            5, // one of the strings is missing null terminator
            "single_ru_missing_nullterm.mo",
            "string_info at 0x84: missing null terminator",
        },
    }
};

// MO loading failure
TEST_CASE( "mo_loading_failure", "[libintl][i18n]" )
{
    for( const auto &it : tests_mo_loading_failures ) {
        CAPTURE( it.serial );
        try {
            trans_catalogue::load_from_file( mo_dir + it.input );
            FAIL_CHECK();
        } catch( std::runtime_error err ) {
            CHECK( err.what() == it.expected );
        }
    }
}

// Load all MO files for the base game to check for loading failures
TEST_CASE( "load_all_base_game_mos", "[libintl][i18n]" )
{
    std::vector<std::string> mo_files = get_files_from_path( ".mo", "lang/mo", true, true );

    if( mo_files.empty() ) {
        WARN( "Skipping (no MO files found)" );
        return;
    }

    for( const std::string &file : mo_files ) {
        try {
            std::vector<trans_catalogue> list;
            list.push_back( trans_catalogue::load_from_file( file ) );
            trans_library lib = trans_library::create( std::move( list ) );
        } catch( std::runtime_error err ) {
            CAPTURE( err.what() );
            FAIL_CHECK();
        }
    }
}
