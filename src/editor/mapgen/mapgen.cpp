#include "mapgen.h"

#include "mapgen/palette.h"
#include "state/state.h"
#include "state/ui_state.h"
#include "widget/widgets.h"

namespace editor
{

static void show_canvas_hint()
{
    ImGui::Text( "Use mouse to paint canvas with palette entries." );
}

void show_mapgen_info( State &state, Mapgen &mapgen, bool &show )
{
    if( !ImGui::Begin( "Mapgen Info", &show ) ) {
        ImGui::End();
        return;
    }
    ImGui::PushID( mapgen.uuid );

    ImGui::Text( "Mapgen type:" );
    if( ImGui::RadioButton( "Oter", mapgen.mtype == MapgenType::Oter ) ) {
        mapgen.mtype = MapgenType::Oter;
        mapgen.base.set_size( mapgen.mapgensize().raw() );
        state.mark_changed();
    }
    ImGui::HelpPopup(
        "Overmap terrain mapgen.\n\n"
        "Must be assigned to one (or more) overmap terrain types.\n"
        "When game generates local map for an omt, it randomly selects one of the overmap mapgens associated "
        "with given omt's type and runs it, then applies automatic transformations such as rotation.\n"
        "Each omt type must have at least 1 omt mapgen assigned to it."
    );
    ImGui::SameLine();
    if( ImGui::RadioButton( "Update", mapgen.mtype == MapgenType::Update ) ) {
        mapgen.mtype = MapgenType::Update;
        mapgen.base.set_size( mapgen.mapgensize().raw() );
        state.mark_changed();
    }
    ImGui::HelpPopup(
        "Update mapgen.\n\n"
        "Invoked by basecamp upgrade routines.\n"
        "Can be used for automatic calculation of camp blueprint requirements."
    );
    ImGui::SameLine();
    if( ImGui::RadioButton( "Nested", mapgen.mtype == MapgenType::Nested ) ) {
        mapgen.mtype = MapgenType::Nested;
        mapgen.base.set_size( mapgen.mapgensize().raw() );
        state.mark_changed();
    }
    ImGui::HelpPopup(
        "Nested mapgen.\n\n"
        "Can be invoked by omt and upgrate mapgens.\n"
        "This is essentially a 'chunk' of any size up to 24x24 that can be procedurally placed by the calling mapgen."
    );
    ImGui::Separator();

    if( ImGui::Button( "Show/hide inline palette" ) ) {
        state.ui->toggle_show_palette( mapgen.base.inline_palette_id );
    }
    ImGui::Separator();

    if( mapgen.mtype == MapgenType::Oter ) {
        if( ImGui::InputId( "om_terrain", mapgen.oter.om_terrain ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Overmap terrain type to assign this mapgen to." );
        if( ImGui::InputIntClamped( "weight", mapgen.oter.weight, 0, 10000 ) ) {
            state.mark_changed( "mapgen-info-oter-weight-input" );
        }
        ImGui::HelpPopup(
            "Weight of this mapgen, defaults to 100.\n\n"
            "The higher this value is, the more frequently this mapgen will be chosen "
            "to generate the overmap terrain."
        );
        if( ImGui::InputIntRange( "rotation", mapgen.oter.rotation ) ) {
            state.mark_changed();
        }
        ImGui::Text( "Oter mapgen base:" );
        ImGui::HelpPopup( "Defines how to fill in the 'empty' tiles in the canvas." );

        if( ImGui::RadioButton( "Fill terrain", mapgen.oter.mapgen_base == OterMapgenBase::FillTer ) ) {
            mapgen.oter.mapgen_base = OterMapgenBase::FillTer;
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Fill with terrain type.\n\n"
            "Useful for maps where most of the terrain is monotonic (e.g. solid rock, or open air)."
        );
        ImGui::SameLine();
        if( ImGui::RadioButton( "Predecessor mapgen",
                                mapgen.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) ) {
            mapgen.oter.mapgen_base = OterMapgenBase::PredecessorMapgen;
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Run this mapgen on top of a map created for some other overmap terrain type.\n\n"
            "Useful for generating objects that don't occupy the whole 24x24 area, "
            "or maps that are extremely similar to some other maps."
            "For example, a small 8x8 glade in the woods may use 'forest' predecessor mapgen "
            "to generate the greenery, and then place some grass in the center.\n\n"
            "Keep in mind that predecessor mapgen may place items, monsters and vehices!"
        );
        ImGui::SameLine();
        if( ImGui::RadioButton( "Rows", mapgen.oter.mapgen_base == OterMapgenBase::Rows ) ) {
            mapgen.oter.mapgen_base = OterMapgenBase::Rows;
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Use a 24x24 canvas to place tiles.\n\n"
            "The most straightforward method, just define a bunch of palettes ('symbol: data' pairs) "
            "and then place symbols on the canvas to define positions.\n"
            "Most useful for complex layouts with little variation, such as buildings."
        );

        if( mapgen.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) {
            if( ImGui::InputId( "predecessor_mapgen", mapgen.oter.predecessor_mapgen ) ) {
                state.mark_changed();
            }
            ImGui::HelpPopup( "Overmap type id to run predecessor mapgen for." );
        } else {
            if( ImGui::InputId( "fill_ter", mapgen.oter.fill_ter ) ) {
                state.mark_changed();
            }
            ImGui::HelpPopup( "Terrain type to fill empty spots with." );
        }
        if( mapgen.oter.mapgen_base == OterMapgenBase::Rows ) {
            show_canvas_hint();
        }
    } else if( mapgen.mtype == MapgenType::Update ) {
        if( ImGui::InputText( "update_mapgen_id", &mapgen.update.update_mapgen_id ) ) {
            state.mark_changed( "update-mapgen-id" );
        }
        ImGui::HelpPopup( "ID of this update mapgen." );
        if( ImGui::InputId( "fill_ter", mapgen.update.fill_ter ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Terrain type to fill empty spots with." );
    } else { // MapgenType::Nested
        if( ImGui::InputText( "nested_mapgen_id", &mapgen.nested.nested_mapgen_id ) ) {
            state.mark_changed( "nested-mapgen-id" );
        }
        ImGui::HelpPopup( "ID of this nested mapgen." );
        if( ImGui::InputIntRange( "rotation", mapgen.nested.rotation ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Allowed rotations." );
        // Only square nested mapgens are possible
        if( ImGui::InputIntClamped( "mapgensize", mapgen.nested.size.x, 1, SEEX * 2 ) ) {
            mapgen.nested.size.y = mapgen.nested.size.x;
            mapgen.base.set_size( mapgen.mapgensize().raw() );
            state.mark_changed();
        }
        ImGui::HelpPopup( "Size of this nested mapgen." );
        show_canvas_hint();
    }

    ImGui::PopID();
    ImGui::End();
}

point_rel_etile Mapgen::mapgensize() const
{
    if( mtype == MapgenType::Nested ) {
        return point_rel_etile( nested.size );
    } else {
        return point_rel_etile( SEEX * 2, SEEY * 2 );
    }
}

MapgenBase::~MapgenBase() = default;

void MapgenBase::remove_usages( const UUID &uuid )
{
    for( UUID &cell : canvas.get_data() ) {
        if( cell == uuid ) {
            cell = UUID_INVALID;
        }
    }
}

} // namespace editor
