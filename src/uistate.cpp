#include "uistate.h"

#include "json.h"

void advanced_inv_pane_save_state::deserialize( const JsonObject &jo, const std::string &prefix )
{
    jo.read( prefix + "sort_idx", sort_idx );
    jo.read( prefix + "filter", filter );
    jo.read( prefix + "area_idx", area_idx );
    jo.read( prefix + "selected_idx", selected_idx );
    jo.read( prefix + "in_vehicle", in_vehicle );
}

void advanced_inv_save_state::deserialize( JsonObject &jo, const std::string &prefix )
{
    jo.read( prefix + "active_left", active_left );
    jo.read( prefix + "last_popup_dest", last_popup_dest );

    jo.read( prefix + "saved_area", saved_area );
    jo.read( prefix + "saved_area_right", saved_area_right );
    pane.area_idx = saved_area;
    pane_right.area_idx = saved_area_right;
    pane.deserialize( jo, prefix + "pane_" );
    pane_right.deserialize( jo, prefix + "pane_right_" );
}
