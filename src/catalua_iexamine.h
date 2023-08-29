#pragma once
#ifndef CATA_SRC_CATALUA_IEXAMINE_H
#define CATA_SRC_CATALUA_IEXAMINE_H

#include <string>

class Character;
struct tripoint;

namespace cata
{
struct lua_state;
}

namespace iexamine
{

bool is_luafunc_valid( cata::lua_state &state, const std::string &funcname );
void run_luafunc( cata::lua_state &state, const std::string &funcname, Character &ch,
                  const tripoint &p, bool is_furn );

} // namespace iexamine

#endif // CATA_SRC_CATALUA_IEXAMINE_H
