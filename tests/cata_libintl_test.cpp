#include "cata_libintl.h"

#include "catch/catch.hpp"
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
            0,
            "n%2",
            "(n%2)",
        },
        {
            1,
            " ( n % 2 ) ",
            "(n%2)",
        },
        {
            2,
            "n?0:1",
            "(n?0:1)",
        },
        {
            3,
            "n?1?2:3:4",
            "(n?(1?2:3):4)",
        },
        {
            4,
            "n==1?n%2:n%3",
            "((n==1)?(n%2):(n%3))",
        },
        {
            5,
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
            11, // Polish
            "n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2",
            "((n==1)?0:((((n%10)>=2)&&(((n%10)<=4)&&(((n%100)<10)||((n%100)>=20))))?1:2))",
        },
        {
            12, // Russian
            "n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2",
            "((((n%10)==1)&&((n%100)!=11))?0:((((n%10)>=2)&&(((n%10)<=4)&&(((n%100)<10)||((n%100)>=20))))?1:2))",
        },
    }
};

static const std::vector<test_case_data> tests_plural_form_rules_fail = {{
        {
            0,
            "n%",
            "expected expression at pos 2",
        },
        {
            1,
            "%2",
            "expected expression at pos 0",
        },
        {
            2,
            "n2",
            "unexpected token at pos 1",
        },
        {
            3,
            " ( n % 2 ",
            "expected closing bracket at pos 9",
        },
        {
            4,
            "  n % 2     )  ",
            "unexpected token at pos 12",
        },
        {
            5,
            "  ",
            "expected expression at pos 2",
        },
        {
            6,
            " ( n % 2 ) 2 % n",
            "unexpected token at pos 11",
        },
        {
            7,
            " ( n % 2 ) % % 4",
            "expected expression at pos 13",
        },
        {
            8,
            "%% 3",
            "expected expression at pos 0",
        },
        {
            9,
            "n % -3",
            "unexpected character '-' at pos 4",
        },
        {
            10,
            "n * 3",
            "unexpected character '*' at pos 2",
        },
        {
            11,
            "(((((n % 3))))))",
            "unexpected token at pos 15",
        },
        {
            12,
            "n % 2 3",
            "unexpected token at pos 6",
        },
        {
            13, // integer overflow
            "n == 4294967296 ? 1 : 0",
            "invalid number '4294967296' at pos 5",
        },
        {
            14,
            "n ? 2 3",
            "expected ternary delimiter at pos 6",
        },
    }
};

TEST_CASE( "Plural forms parser", "[libintl][i18n]" )
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
        } catch( std::runtime_error e ) {
            CHECK( e.what() == it.expected );
        }
    }
}

TEST_CASE( "Plural forms calculation", "[libintl][i18n]" )
{
    constexpr size_t CHECK_STEP = 32;
    constexpr size_t CHECK_TOTAL = 1'000'000;
    constexpr size_t CHECK_RANDOM = 1'000'000;

    // Speed is ~ 1'000'000 / second

    std::vector<std::string> strs = {{
            "предмет", // one
            "предмета", // few
            "предметов", // many
            "предметы", // other (fractional)
        }
    };

    std::string test1 =
        "n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2";
    std::string test2 =
        "(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 : n%10==0 || (n%10>=5 && n%10<=9) || (n%100>=11 && n%100<=14)? 2 : 3)";

    cata_internal::PlfNodePtr ptr1 = cata_internal::parse_plural_rules( test1 );
    cata_internal::PlfNodePtr ptr2 = cata_internal::parse_plural_rules( test2 );

    // Ordered check
    cata_printf( "Started 1st PLF check.\n" );
    bool printed = false;
    for( size_t x = 0; x < CHECK_TOTAL; x++ ) {
        int plf1 = ptr1->eval( x );
        int plf2 = ptr2->eval( x );
        if( plf1 != plf2 ) {
            cata_printf( " x=% 6d  plf1=%d  plf2=%d  % 6d %s\n", x, plf1, plf2, x, strs[plf2] );
            printed = true;
        }
        if( printed && x % CHECK_STEP == 0 ) {
            break;
        }
    }

    // Random check
    cata_printf( "Started 2nd PLF check.\n" );
    for( size_t i = 0; i < CHECK_RANDOM; i++ ) {
        static std::uniform_int_distribution<unsigned long> rng_uint_dist;
        unsigned long x = rng_uint_dist( rng_get_engine() );

        int plf1 = ptr1->eval( x );
        int plf2 = ptr2->eval( x );
        if( plf1 != plf2 ) {
            cata_printf( " x=% 6d  plf1=%d  plf2=%d  % 6d %s\n", x, plf1, plf2, x, strs[plf2] );
            break;
        }
    }

    cata_printf( "Done with PLF checks.\n" );
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

TEST_CASE( "Load single MO and get strings", "[libintl][i18n]" )
{
    SECTION( "Little endian file" ) {
        trans_library lib;
        lib.add_catalogue( trans_catalogue::load_from_file( mo_dir + "single_ru_little_endian.mo" ) );
        lib.finalize();

        test_get_strings( lib );
    }
    SECTION( "Big endian file" ) {
        trans_library lib;
        lib.add_catalogue( trans_catalogue::load_from_file( mo_dir + "single_ru_big_endian.mo" ) );
        lib.finalize();

        test_get_strings( lib );
    }
}

TEST_CASE( "Load multiple MO and get strings", "[libintl][i18n]" )
{
    trans_library lib;
    lib.add_catalogue( trans_catalogue::load_from_file( mo_dir + "multi_1_ru.mo" ) );
    lib.add_catalogue( trans_catalogue::load_from_file( mo_dir + "multi_2_ru.mo" ) );
    lib.add_catalogue( trans_catalogue::load_from_file( mo_dir + "multi_3_ru.mo" ) );
    lib.finalize();

    test_get_strings( lib );
}

static const std::vector<test_case_data> tests_mo_loading_failures = {{
        {
            0, // file not found
            "non-existent.mo",
            "failed to open file",
        },
        {
            1, // not a file
            "",
            "not a MO file", // FIXME: this should be "failed to open file"
        },
        {
            2, // not a MO file (magic number mismatch)
            "single.pot",
            "not a MO file",
        },
        {
            3, // not a MO file (too small to have magic number)
            "empty_file.mo",
            "not a MO file",
        },
        {
            4, // wrong charset (only UTF-8 is supported)
            "wrong_charset_ru.mo",
            "unexpected value in Content-Type header (wrong charset?)",
        },
        {
            5, // one of the strings extends beyond end of file
            "single_ru_string_ignores_eof.mo",
            "string_info at 0x84: extends beyond EOF (len:0x16 addr:0x35f file size:0x375)",
        },
        {
            6, // one of the strings is missing null terminator
            "single_ru_missing_nullterm.mo",
            "string_info at 0x84: missing null terminator",
        },
    }
};

TEST_CASE( "MO loading failure", "[libintl][i18n]" )
{
    for( const auto &it : tests_mo_loading_failures ) {
        CAPTURE( it.serial );
        try {
            trans_catalogue::load_from_file( mo_dir + it.input );
            FAIL_CHECK();
        } catch( std::runtime_error e ) {
            CHECK( e.what() == it.expected );
        }
    }
}
