#include "catalua_iexamine.h"

#if defined(LUA)

#include "catalua_iuse_actor.h"

#include "catalua_sol.h"
#include "catalua_impl.h"
#include "character.h"

namespace iexamine
{

bool is_luafunc_valid( cata::lua_state &state, const std::string &funcname )
{
    sol::state &lua = state.lua;
    sol::table funcs = lua.globals()["game"]["examine_functions"];
    try {
        sol::reference ref = funcs[funcname];
        if( !ref ) {
            return false;
        }
        // Try to cast into function
        sol::protected_function func = ref;
        ( void )func;
        return true;
    } catch( std::runtime_error &e ) {
        debugmsg( "Failed to check for examine_function k='%s': %s", funcname, e.what() );
    }
    return false;
}

void run_luafunc( cata::lua_state &state, const std::string &funcname, Character &ch,
                  const tripoint &p, bool is_furn )
{
    sol::state &lua = state.lua;
    sol::table funcs = lua.globals()["game"]["examine_functions"];
    try {
        sol::protected_function func = funcs[funcname];
        sol::protected_function_result res = func( ch, p, is_furn );
        check_func_result( res );
    } catch( std::runtime_error &e ) {
        debugmsg( "Failed to run examine_function k='%s': %s", funcname, e.what() );
    }
}

} // namespace iexamine

#else // LUA

namespace iexamine
{
bool is_luafunc_valid( const std::string & )
{
    return false;
}
void run_luafunc( cata::lua_state &, const std::string &, Character &, const tripoint &, bool ) {}
} // namespace iexamine

#endif // LUA
