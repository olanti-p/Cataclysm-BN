#include "uistate.h"

#include "json.h"

void advanced_inv_pane_save_state::serialize( JsonOut &json, const std::string &prefix ) const
{
    json.member( prefix + "sort_idx", sort_idx );
    json.member( prefix + "filter", filter );
    json.member( prefix + "area_idx", area_idx );
    json.member( prefix + "selected_idx", selected_idx );
    json.member( prefix + "in_vehicle", in_vehicle );
}

void advanced_inv_pane_save_state::deserialize( const JsonObject &jo, const std::string &prefix )
{

    jo.read( prefix + "sort_idx", sort_idx );
    jo.read( prefix + "filter", filter );
    jo.read( prefix + "area_idx", area_idx );
    jo.read( prefix + "selected_idx", selected_idx );
    jo.read( prefix + "in_vehicle", in_vehicle );
}

void advanced_inv_save_state::serialize( JsonOut &json, const std::string &prefix ) const
{
    json.member( prefix + "active_left", active_left );
    json.member( prefix + "last_popup_dest", last_popup_dest );

    json.member( prefix + "saved_area", saved_area );
    json.member( prefix + "saved_area_right", saved_area_right );
    pane.serialize( json, prefix + "pane_" );
    pane_right.serialize( json, prefix + "pane_right_" );
}

void advanced_inv_save_state::deserialize( const JsonObject &jo, const std::string &prefix )
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

void uistatedata::serialize( JsonOut &json ) const
{
    const unsigned int input_history_save_max = 25;
    json.start_object();

    transfer_save.serialize( json, "transfer_save_" );

    /**** if you want to save whatever so it's whatever when the game is started next, declare here and.... ****/
    // non array stuffs
    json.member( "adv_inv_container_location", adv_inv_container_location );
    json.member( "adv_inv_container_index", adv_inv_container_index );
    json.member( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
    json.member( "adv_inv_container_type", adv_inv_container_type );
    json.member( "adv_inv_container_content_type", adv_inv_container_content_type );
    json.member( "editmap_nsa_viewmode", editmap_nsa_viewmode );
    json.member( "overmap_blinking", overmap_blinking );
    json.member( "overmap_show_overlays", overmap_show_overlays );
    json.member( "overmap_show_map_notes", overmap_show_map_notes );
    json.member( "overmap_show_land_use_codes", overmap_show_land_use_codes );
    json.member( "overmap_show_city_labels", overmap_show_city_labels );
    json.member( "overmap_show_hordes", overmap_show_hordes );
    json.member( "overmap_show_forest_trails", overmap_show_forest_trails );
    json.member( "vmenu_show_items", vmenu_show_items );
    json.member( "list_item_sort", list_item_sort );
    json.member( "list_item_filter_active", list_item_filter_active );
    json.member( "list_item_downvote_active", list_item_downvote_active );
    json.member( "list_item_priority_active", list_item_priority_active );
    json.member( "hidden_recipes", hidden_recipes );
    json.member( "favorite_recipes", favorite_recipes );
    json.member( "recent_recipes", recent_recipes );

    json.member( "input_history" );
    json.start_object();
    for( auto &e : input_history ) {
        json.member( e.first );
        const std::vector<std::string> &history = e.second;
        json.start_array();
        int save_start = 0;
        if( history.size() > input_history_save_max ) {
            save_start = history.size() - input_history_save_max;
        }
        for( std::vector<std::string>::const_iterator hit = history.begin() + save_start;
             hit != history.end(); ++hit ) {
            json.write( *hit );
        }
        json.end_array();
    }
    json.end_object(); // input_history

    json.end_object();
}

void uistatedata::deserialize( const JsonObject &jo )
{
    transfer_save.deserialize( jo, "transfer_save_" );

    // the rest
    jo.read( "adv_inv_container_location", adv_inv_container_location );
    jo.read( "adv_inv_container_index", adv_inv_container_index );
    jo.read( "adv_inv_container_in_vehicle", adv_inv_container_in_vehicle );
    jo.read( "adv_inv_container_type", adv_inv_container_type );
    jo.read( "adv_inv_container_content_type", adv_inv_container_content_type );
    jo.read( "overmap_blinking", overmap_blinking );
    jo.read( "overmap_show_overlays", overmap_show_overlays );
    jo.read( "overmap_show_map_notes", overmap_show_map_notes );
    jo.read( "overmap_show_land_use_codes", overmap_show_land_use_codes );
    jo.read( "overmap_show_city_labels", overmap_show_city_labels );
    jo.read( "overmap_show_hordes", overmap_show_hordes );
    jo.read( "overmap_show_forest_trails", overmap_show_forest_trails );
    jo.read( "hidden_recipes", hidden_recipes );
    jo.read( "favorite_recipes", favorite_recipes );
    jo.read( "recent_recipes", recent_recipes );

    if( !jo.read( "vmenu_show_items", vmenu_show_items ) ) {
        // This is an old save: 1 means view items, 2 means view monsters,
        // -1 means uninitialized
        vmenu_show_items = jo.get_int( "list_item_mon", -1 ) != 2;
    }

    jo.read( "list_item_sort", list_item_sort );
    jo.read( "list_item_filter_active", list_item_filter_active );
    jo.read( "list_item_downvote_active", list_item_downvote_active );
    jo.read( "list_item_priority_active", list_item_priority_active );

    for( const JsonMember member : jo.get_object( "input_history" ) ) {
        std::vector<std::string> &v = gethistory( member.name() );
        v.clear();
        for( const std::string line : member.get_array() ) {
            v.push_back( line );
        }
    }
    // fetch list_item settings from input_history
    if( !gethistory( "item_filter" ).empty() ) {
        list_item_filter = gethistory( "item_filter" ).back();
    }
    if( !gethistory( "list_item_downvote" ).empty() ) {
        list_item_downvote = gethistory( "list_item_downvote" ).back();
    }
    if( !gethistory( "list_item_priority" ).empty() ) {
        list_item_priority = gethistory( "list_item_priority" ).back();
    }
}
