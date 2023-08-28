#ifdef LUA
#include "catalua_bindings.h"

#include "catalua_bindings_utils.h"
#include "catalua_luna_doc.h"
#include "catalua_luna.h"
#include "overmap.h"
#include "overmap_special.h"

void cata::detail::reg_overmap_api( sol::state &lua )
{
    {
        sol::usertype<overmap> ut =
            luna::new_usertype<overmap>(
                lua,
                luna::no_bases,
                luna::no_constructor
            );

        luna::set_fx( ut, "get_abs_pos", []( const overmap & om ) -> point {
            return om.pos().raw();
        } );

        luna::set_fx( ut, "get_nearest_city", []( const overmap & om, const tripoint & p ) -> const city& {
            return om.get_nearest_city( tripoint_om_omt( p ) );
        } );

        DOC(
            "Boolean argument controls whether the overmap area must be unexplored to be considered valid. \n"
            "Coords are within the overmap itself."
        );
        luna::set_fx( ut, "can_place_special",
                      [](
                          const overmap & om,
                          const overmap_special & special,
                          const tripoint & p,
                          om_direction::type dir,
                          bool must_be_unexplored
        ) -> bool {
            return om.can_place_special( special, tripoint_om_omt( p ), dir, must_be_unexplored );
        } );

        DOC(
            "1st boolean argument controls whether the overmap area must be unexplored to be considered valid. \n"
            "Setting 2nd boolean argument to true will overrides all checks and force the placement. \n"
            "Coords are within the overmap itself."
        );
        luna::set_fx( ut, "place_special",
                      [](
                          overmap & om,
                          const overmap_special & special,
                          const tripoint & p,
                          om_direction::type dir,
                          const city & cit,
                          bool must_be_unexplored,
                          bool force
        ) {
            om.place_special( special, tripoint_om_omt( p ), dir, cit, must_be_unexplored, force );
        } );
    }
}

#endif
