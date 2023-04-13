#ifdef LUA
#include "catalua_bindings.h"

#include "catalua_bindings_utils.h"
#include "catalua_luna_doc.h"
#include "catalua_luna.h"
#include "coordinates.h"
#include "json.h"
#include "line.h"
#include "point.h"

void cata::detail::reg_point_tripoint( sol::state &lua )
{
    // Register 'point' class to be used in Lua
    {
        sol::usertype<point> ut =
            luna::new_usertype<point>(
                lua,
                luna::no_bases,
                luna::constructors <
                point(),
                point( const point & ),
                point( int, int )
                > ()
            );

        // Members
        luna::set( ut, "x", &point::x );
        luna::set( ut, "y", &point::y );

        // Methods
        luna::set_fx( ut, "abs", &point::abs );
        luna::set_fx( ut, "rotate", &point::rotate );

        // (De-)Serialization
        reg_serde_functions( ut );

        // To string
        // We're using Lua meta function here to make it work seamlessly with native Lua tostring()
        luna::set_fx( ut, sol::meta_function::to_string, &point::to_string );

        // Equality operator
        // It's defined as inline friend function inside point class, we can't access it and so have to improvise
        luna::set_fx( ut, sol::meta_function::equal_to, []( const point & a, const point & b ) {
            return a == b;
        } );

        // Less-then operator
        // Same deal as with equality operator
        luna::set_fx( ut, sol::meta_function::less_than, []( const point & a, const point & b ) {
            return a < b;
        } );

        // Arithmetic operators
        // point + point
        luna::set_fx( ut, sol::meta_function::addition, &point::operator+ );
        // point - point
        // sol::resolve here makes it possible to specify which overload to use
        luna::set_fx( ut, sol::meta_function::subtraction, sol::resolve< point( point ) const >
                      ( &point::operator- ) );
        // point * int
        luna::set_fx( ut, sol::meta_function::multiplication, &point::operator* );
        // point / float
        luna::set_fx( ut, sol::meta_function::division, &point::operator/ );
        // point / int
        luna::set_fx( ut, sol::meta_function::floor_division, &point::operator/ );
        // -point
        // sol::resolve here makes it possible to specify which overload to use
        luna::set_fx( ut, sol::meta_function::unary_minus,
                      sol::resolve< point() const >( &point::operator- ) );
    }

    // Register 'tripoint' class to be used in Lua
    {
        sol::usertype<tripoint> ut =
            luna::new_usertype<tripoint>(
                lua,
                luna::no_bases,
                luna::constructors <
                tripoint(),
                tripoint( const point &, int ),
                tripoint( const tripoint & ),
                tripoint( int, int, int )
                > ()
            );

        // Members
        luna::set( ut, "x", &tripoint::x );
        luna::set( ut, "y", &tripoint::y );
        luna::set( ut, "z", &tripoint::z );

        // Methods
        luna::set_fx( ut, "abs", &tripoint::abs );
        luna::set_fx( ut, "xy", &tripoint::xy );
        luna::set_fx( ut, "rotate_2d", &tripoint::rotate_2d );

        // (De-)Serialization
        reg_serde_functions( ut );

        // To string
        // We're using Lua meta function here to make it work seamlessly with native Lua tostring()
        luna::set_fx( ut, sol::meta_function::to_string, &tripoint::to_string );

        // Equality operator
        // It's defined as inline friend function inside point class, we can't access it and so have to improvise
        luna::set_fx( ut, sol::meta_function::equal_to, []( const tripoint & a, const tripoint & b ) {
            return a == b;
        } );

        // Less-then operator
        // Same deal as with equality operator
        luna::set_fx( ut, sol::meta_function::less_than, []( const tripoint & a, const tripoint & b ) {
            return a < b;
        } );

        // Arithmetic operators
        // tripoint + tripoint (overload 1)
        // tripoint + point (overload 2)
        luna::set_fx( ut, sol::meta_function::addition, sol::overload(
                          sol::resolve< tripoint( const tripoint & ) const > ( &tripoint::operator+ ),
                          sol::resolve< tripoint( point ) const > ( &tripoint::operator+ )
                      ) );
        // tripoint - tripoint (overload 1)
        // tripoint - point (overload 2)
        luna::set_fx( ut, sol::meta_function::subtraction, sol::overload(
                          sol::resolve< tripoint( const tripoint & ) const > ( &tripoint::operator- ),
                          sol::resolve< tripoint( point ) const > ( &tripoint::operator- )
                      ) );
        // tripoint * int
        luna::set_fx( ut, sol::meta_function::multiplication, &tripoint::operator* );
        // tripoint / float
        luna::set_fx( ut, sol::meta_function::division, &tripoint::operator/ );
        // tripoint / int
        luna::set_fx( ut, sol::meta_function::floor_division, &tripoint::operator/ );
        // -tripoint
        // sol::resolve here makes it possible to specify which overload to use
        luna::set_fx( ut, sol::meta_function::unary_minus,
                      sol::resolve< tripoint() const >( &tripoint::operator- ) );
    }
}

void cata::detail::reg_coords_library( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "coords" );

    luna::set_fx( lib, "ms_to_sm", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_sm rough;
        point_sm_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );
    luna::set_fx( lib, "ms_to_omt", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_omt rough;
        point_omt_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );
    luna::set_fx( lib, "ms_to_om", []( const tripoint & raw ) -> std::tuple<point, tripoint> {
        tripoint_rel_ms fine( raw );
        point_rel_om rough;
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
        std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );

    luna::set_fx( lib, "sm_to_ms", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_sm rough( raw_rough );
        point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );
    luna::set_fx( lib, "omt_to_ms", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_omt rough( raw_rough );
        point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );
    luna::set_fx( lib, "om_to_ms", []( const point & raw_rough,
    sol::optional<const tripoint &> raw_remain ) -> tripoint {
        point_rel_om rough( raw_rough );
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
            raw_remain ? *raw_remain : tripoint_zero
        );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );

    luna::set_fx( lib, "rl_dist", sol::overload(
                      sol::resolve<int( const tripoint &, const tripoint & )>( rl_dist ),
                      sol::resolve<int( point, point )>( rl_dist )
                  ) );
    luna::set_fx( lib, "trig_dist", sol::overload(
                      sol::resolve<float( const tripoint &, const tripoint & )>( trig_dist ),
                      sol::resolve<float( point, point )>( trig_dist )
                  ) );
    luna::set_fx( lib, "square_dist", sol::overload(
                      sol::resolve<int( const tripoint &, const tripoint & )>( square_dist ),
                      sol::resolve<int( point, point )>( square_dist )
                  ) );

    luna::finalize_lib( lib );
}

void cata::detail::reg_test_library_1( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "test1" );

    luna::set_fx( lib, "f1", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_sm rough;
        point_sm_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );
    luna::set_fx( lib, "f2", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_omt rough;
        point_omt_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );
    luna::set_fx( lib, "f3", []( const tripoint & raw ) -> std::tuple<point, tripoint> {
        tripoint_rel_ms fine( raw );
        point_rel_om rough;
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
        std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );

    luna::set_fx( lib, "f4", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_sm rough( raw_rough );
        point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );
    luna::set_fx( lib, "f5", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_omt rough( raw_rough );
        point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );
    luna::set_fx( lib, "f6", []( const point & raw_rough,
    sol::optional<const tripoint &> raw_remain ) -> tripoint {
        point_rel_om rough( raw_rough );
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
            raw_remain ? *raw_remain : tripoint_zero
        );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );

    luna::finalize_lib( lib );
}

void cata::detail::reg_test_library_2( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "test2" );

    auto f1 = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_sm rough;
        point_sm_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };
    luna::set_fx( lib, "f1", f1 );

    auto f2 = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_omt rough;
        point_omt_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };
    luna::set_fx( lib, "f2", f2 );

    auto f3 = []( const tripoint & raw ) -> std::tuple<point, tripoint> {
        tripoint_rel_ms fine( raw );
        point_rel_om rough;
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
        std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };
    luna::set_fx( lib, "f3", f3 );

    auto f4 = []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_sm rough( raw_rough );
        point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };
    luna::set_fx( lib, "f4", f4 );

    auto f5 = []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_omt rough( raw_rough );
        point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };
    luna::set_fx( lib, "f5", f5 );

    auto f6 = []( const point & raw_rough,
    sol::optional<const tripoint &> raw_remain ) -> tripoint {
        point_rel_om rough( raw_rough );
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
            raw_remain ? *raw_remain : tripoint_zero
        );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };
    luna::set_fx( lib, "f6", f6 );

    luna::finalize_lib( lib );
}

void cata::detail::reg_test_library_3( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "test3" );

    {
        auto f1 = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
            tripoint_rel_ms fine( raw );
            tripoint_rel_sm rough;
            point_sm_ms remain;
            std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
            return std::make_pair( rough.raw(), remain.raw() );
        };
        luna::set_fx( lib, "f1", f1 );
    }

    {
        auto f2 = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
            tripoint_rel_ms fine( raw );
            tripoint_rel_omt rough;
            point_omt_ms remain;
            std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
            return std::make_pair( rough.raw(), remain.raw() );
        };
        luna::set_fx( lib, "f2", f2 );
    }

    {
        auto f3 = []( const tripoint & raw ) -> std::tuple<point, tripoint> {
            tripoint_rel_ms fine( raw );
            point_rel_om rough;
            coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
            std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
            return std::make_pair( rough.raw(), remain.raw() );
        };
        luna::set_fx( lib, "f3", f3 );
    }

    {
        auto f4 = []( const tripoint & raw_rough,
        sol::optional<const point &> raw_remain ) -> tripoint {
            tripoint_rel_sm rough( raw_rough );
            point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
            tripoint_rel_ms fine = coords::project_combine( rough, remain );
            return fine.raw();
        };
        luna::set_fx( lib, "f4", f4 );
    }

    {
        auto f5 = []( const tripoint & raw_rough,
        sol::optional<const point &> raw_remain ) -> tripoint {
            tripoint_rel_omt rough( raw_rough );
            point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
            tripoint_rel_ms fine = coords::project_combine( rough, remain );
            return fine.raw();
        };
        luna::set_fx( lib, "f5", f5 );
    }

    {
        auto f6 = []( const point & raw_rough,
        sol::optional<const tripoint &> raw_remain ) -> tripoint {
            point_rel_om rough( raw_rough );
            coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
                raw_remain ? *raw_remain : tripoint_zero
            );
            tripoint_rel_ms fine = coords::project_combine( rough, remain );
            return fine.raw();
        };
        luna::set_fx( lib, "f6", f6 );
    }

    luna::finalize_lib( lib );
}

static std::tuple<tripoint, point>
f1( const tripoint &raw )
{
    tripoint_rel_ms fine( raw );
    tripoint_rel_sm rough;
    point_sm_ms remain;
    std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
    return std::make_pair( rough.raw(), remain.raw() );
}

static std::tuple<tripoint, point>
f2( const tripoint &raw )
{
    tripoint_rel_ms fine( raw );
    tripoint_rel_omt rough;
    point_omt_ms remain;
    std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
    return std::make_pair( rough.raw(), remain.raw() );
}

static std::tuple<point, tripoint>
f3( const tripoint &raw )
{
    tripoint_rel_ms fine( raw );
    point_rel_om rough;
    coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
    std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
    return std::make_pair( rough.raw(), remain.raw() );
}

static tripoint
f4( const tripoint &raw_rough, sol::optional<const point &> raw_remain )
{
    tripoint_rel_sm rough( raw_rough );
    point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
    tripoint_rel_ms fine = coords::project_combine( rough, remain );
    return fine.raw();
}

static tripoint
f5( const tripoint &raw_rough, sol::optional<const point &> raw_remain )
{
    tripoint_rel_omt rough( raw_rough );
    point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
    tripoint_rel_ms fine = coords::project_combine( rough, remain );
    return fine.raw();
}

static tripoint
f6( const point &raw_rough, sol::optional<const tripoint &> raw_remain )
{
    point_rel_om rough( raw_rough );
    coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
        raw_remain ? *raw_remain : tripoint_zero
    );
    tripoint_rel_ms fine = coords::project_combine( rough, remain );
    return fine.raw();
}

void cata::detail::reg_test_library_4( sol::state &lua )
{
    luna::userlib lib = luna::begin_lib( lua, "test4" );

    luna::set_fx( lib, "f1", f1 );
    luna::set_fx( lib, "f2", f2 );
    luna::set_fx( lib, "f3", f3 );
    luna::set_fx( lib, "f4", f4 );
    luna::set_fx( lib, "f5", f5 );
    luna::set_fx( lib, "f6", f6 );

    luna::finalize_lib( lib );
}

void cata::detail::reg_test_library_5( sol::state &lua )
{
    sol::table lib = lua.create_table();
    lua.globals()["test5"] = lib;

    lib.set( "f1", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_sm rough;
        point_sm_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );

    lib.set( "f2", []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_omt rough;
        point_omt_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );

    lib.set( "f3", []( const tripoint & raw ) -> std::tuple<point, tripoint> {
        tripoint_rel_ms fine( raw );
        point_rel_om rough;
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
        std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    } );

    lib.set( "f4", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_sm rough( raw_rough );
        point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );

    lib.set( "f5", []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_omt rough( raw_rough );
        point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );

    lib.set( "f6", []( const point & raw_rough,
    sol::optional<const tripoint &> raw_remain ) -> tripoint {
        point_rel_om rough( raw_rough );
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
            raw_remain ? *raw_remain : tripoint_zero
        );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    } );
}

void cata::detail::reg_test_library_6( sol::state &lua )
{
    sol::table lib = lua.create_table();
    lua.globals()["test6"] = lib;

    lib["f1"] = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_sm rough;
        point_sm_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::sm>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };

    lib["f2"] = []( const tripoint & raw ) -> std::tuple<tripoint, point> {
        tripoint_rel_ms fine( raw );
        tripoint_rel_omt rough;
        point_omt_ms remain;
        std::tie( rough, remain ) = coords::project_remain<coords::omt>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };

    lib["f3"] = []( const tripoint & raw ) -> std::tuple<point, tripoint> {
        tripoint_rel_ms fine( raw );
        point_rel_om rough;
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain;
        std::tie( rough, remain ) = coords::project_remain<coords::om>( fine );
        return std::make_pair( rough.raw(), remain.raw() );
    };

    lib["f4"] = []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_sm rough( raw_rough );
        point_sm_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };

    lib["f5"] = []( const tripoint & raw_rough,
    sol::optional<const point &> raw_remain ) -> tripoint {
        tripoint_rel_omt rough( raw_rough );
        point_omt_ms remain( raw_remain ? *raw_remain : point_zero );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };

    lib["f6"] = []( const point & raw_rough,
    sol::optional<const tripoint &> raw_remain ) -> tripoint {
        point_rel_om rough( raw_rough );
        coords::coord_point<tripoint, coords::origin::overmap, coords::ms> remain(
            raw_remain ? *raw_remain : tripoint_zero
        );
        tripoint_rel_ms fine = coords::project_combine( rough, remain );
        return fine.raw();
    };
}

#endif
