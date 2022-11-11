#include "editor_me_state.h"
#include "editor_widgets.h"
#include "editor_me_state_export.h"
#include "editor_me_canvas.h"

#include "../fstream_utils.h"
#include "../game_constants.h"
#include "../string_utils.h"
#include "../text_snippets.h"

#ifdef DebugLog
#  undef DebugLog
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ImGuiFileDialog.h"

#include <unordered_set>

namespace editor
{
static void handle_file_saving( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }

    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_S ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) || !state.file_save_path ) {
            state.open_save_as = true;
        } else {
            state.do_save = true;
        }
    }

    if( state.open_save_as ) {
        state.open_save_as = false;
        ImGuiFileDialog::Instance()->OpenDialog( "SaveToFile",
                "Save As...", ".json",
                state.file_save_path ? *state.file_save_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "SaveToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            state.file_save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            state.do_save = true;
        } else {
            state.do_exit_after_save = false;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( state.do_save ) {
        state.do_save = false;
        assert( state.file_save_path );
        write_to_file( *state.file_save_path, [&]( std::ostream & oss ) {
            oss << serialize( state.file() );
        } );
        state.last_saved_revision = state.current_revision.num;
        if( state.do_exit_after_save ) {
            state.do_loop = false;
        }
    }
}

static void handle_file_exporting( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }

    if( state.open_export_as ) {
        state.open_export_as = false;
        ImGuiFileDialog::Instance()->OpenDialog( "ExportToFile",
                "Export As...", ".json",
                state.file_export_path ? *state.file_export_path : ".",
                1, nullptr, ImGuiFileDialogFlags_ConfirmOverwrite );
    }

    if( ImGuiFileDialog::Instance()->Display( "ExportToFile" ) ) {
        if( ImGuiFileDialog::Instance()->IsOk() ) {
            state.file_export_path = ImGuiFileDialog::Instance()->GetFilePathName();
            state.do_export = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if( state.do_export ) {
        state.do_export = false;
        assert( state.file_export_path );
        write_to_file( *state.file_export_path, [&]( std::ostream & oss ) {
            std::string s = editor_export::to_string( state.file() );
            oss << editor_export::format_string( s );
        } );
        state.last_exported_revision = state.current_revision.num;
    }
}

void show_control_window( me_state &state )
{
    bool keep_open = true;
    ImGui::Begin( "Advanced Map Editor", &keep_open );
    ImGui::Text( "Close this window to close the project." );

    if( !keep_open ) {
        if( state.has_unsaved_changes() ) {
            ImGui::OpenPopup( "###warn-unsaved-on-close" );
        } else {
            state.do_loop = false;
        }
    }

    if( ImGui::BeginPopupModal( "###warn-unsaved-on-close", nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize ) ) {
        ImGui::TextCentered( "Do you want to save the changes?" );
        ImGui::Text( " " );
        ImVec2 btn_sz( ImGui::GetFrameHeight() * 5.0f, ImGui::GetFrameHeight() );
        if( ImGui::Button( "Don't Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            state.do_loop = false;
        }
        ImGui::SameLine();
        if( ImGui::Button( "Cancel", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if( ImGui::Button( "Save", btn_sz ) ) {
            ImGui::CloseCurrentPopup();
            state.do_exit_after_save = true;
            if( state.file_save_path ) {
                state.do_save = true;
            } else {
                state.open_save_as = true;
            }
        }
        ImGui::EndPopup();
    }

    // Controls
    if( ImGui::Button( "Toggle Demo Window" ) ) {
        state.show_demo_wnd = !state.show_demo_wnd;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Asset Library" ) ) {
        state.show_asset_lib = !state.show_asset_lib;
    }

    if( ImGui::Button( "Toggle File Info" ) ) {
        state.show_file_info = !state.show_file_info;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle History" ) ) {
        state.show_file_history = !state.show_file_history;
    }
    ImGui::SameLine();
    if( ImGui::Button( "Toggle Toolbar" ) ) {
        state.show_toolbar = !state.show_toolbar;
    }

    std::string save_btn = string_format( "%sSave###save-button",
                                          state.has_unsaved_changes() ? "* " : "" );
    if( ImGui::Button( save_btn.c_str() ) ) {
        if( !state.file_save_path ) {
            state.open_save_as = true;
        } else {
            state.do_save = true;
        }
    }
    ImGui::SameLine();
    if( ImGui::Button( "Save As..." ) ) {
        state.open_save_as = true;
    }

    handle_file_saving( state );

    std::string export_btn = string_format( "%sExport###export-button",
                                            state.has_unexported_changes() ? "* " : "" );
    if( ImGui::Button( export_btn.c_str() ) ) {
        if( !state.file_export_path ) {
            state.open_export_as = true;
        } else {
            state.do_export = true;
        }
    }
    ImGui::SameLine();
    if( ImGui::Button( "Export As..." ) ) {
        state.open_export_as = true;
    }

    handle_file_exporting( state );

    // Camera
    {
        ImGui::TextDisabled( "(?: Camera contols)" );
        ImGui::HelpPopup(
            "Camera controls:\n\n"
            "- Drag the view with RMB to pan.\n"
            "- Scroll over the view to zoom.\n"
            "- Use widgets below to manually control zoom and position.\n"
            "\nIn canvas mode:\n"
            "- Press MMB (mouse wheel) on tile to select it.\n"
            "- Press MMB outside bounds (or on empty tile) to clear selection."
        );
        ImGui::DragInt( "Zoom", &state.camera.scale, 0.2f, MIN_SCALE, MAX_SCALE );
        ImGui::DragPoint( "Pos", &state.camera.pos, 1.0f, -10000, 10000 );
    }

    // Mouse position
    {
        point_abs_screen screen_pos = get_mouse_pos();
        point_abs_etile etile_pos = get_mouse_tile_pos( state.camera );
        ImGui::Text( "Mouse pos, px: %s", screen_pos.to_string().c_str() );
        ImGui::Text( "Mouse pos, tile: %s", etile_pos.to_string().c_str() );
    }

    ImGui::End();
}

void show_file_history( me_state &state, bool &show )
{
    if( !ImGui::Begin( "File history", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::HelpMarkerInline(
        "Undo/redo support.\n\n"
        "In order to enable undo and redo, the editor has to keep track of the old versions (revisions) of the file.  "
        "This is done entirely in memory, so remembering too much revisions may exhaust available RAM at some point "
        "and trigger program termination by the OS.  You can manually control how much revisions will be kept alive "
        "using the widget below.\n"
        "\nHotkeys:\n"
        "  Ctrl+Z - Undo (advance to older revision)\n"
        "  Ctrl+Shift+Z - Redo (advance to newer revision)\n"
    );

    ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 4.0f );
    ImGui::InputIntClamped( "History limit", state.history_capacity, 10, 10000,
                            ImGuiInputTextFlags_AutoSelectAll );

    ImGui::HelpMarkerInline(
        "The list below keeps track of file revisions.\n\n"
        "Click on a revision to make it active.  "
        "Every edit automatically generates a new revision and places it at the top.\n"
        "\nMarkers use in the list:\n"
        "  [S] This revision is the one saved in the project file.\n"
        "  [E] This revision is the one that was used for export.\n"
    );
    ImGui::Text( "Edit counter (debug): %d", state.edit_counter );

    for( const me_file_revision &entry : state.file_history ) {
        bool is_saved = state.last_saved_revision && *state.last_saved_revision == entry.num;
        bool is_exported = state.last_exported_revision && *state.last_exported_revision == entry.num;
        std::string fname = string_format(
                                "Version %d%s%s",
                                entry.num,
                                is_saved ? " [S]" : "",
                                is_exported ? " [E]" : ""
                            );
        if( ImGui::Selectable( fname.c_str(), entry.num == state.current_revision.num ) ) {
            state.switch_to_revision = entry.num;
        }
    }

    ImGui::End();
}

static bool filter_matches( const std::string &s, const std::string &filter )
{
    return lcmatch( s, filter );
}

static void show_assetlib_tab( asset_library_cat &cat, asset_library &assets )
{
    const char *cat_name = get_asset_type_name( cat.atype );
    if( !ImGui::BeginTabItem( cat_name ) ) {
        cat.is_active_tab = false;
        return;
    }
    cat.is_active_tab = true;

    ImGui::Text( "%s - %d entries", cat_name, cat.get_num() );

    ImGui::InputText( "##filter", &cat.filter );
    std::string lb_name = string_format( "##lb-%s", cat_name );
    if( ImGui::BeginListBox( lb_name.c_str(), ImVec2( -1.0f, -1.0f ) ) ) {
        for( int i = 0; i < cat.get_num(); i++ ) {
            const asset_lib_entry &entry = cat.get( i );
            const char *id = entry.get_id();
            if( !filter_matches( id, cat.filter ) ) {
                continue;
            }
            const bool is_selected = i == cat.selected;
            if( ImGui::Selectable( id, is_selected ) ) {
                cat.selected = i;
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if( is_selected ) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }

    ImGui::EndTabItem();
}

void show_asset_lib( asset_library &assets, bool &show )
{
    if( ImGui::Begin( "Asset Library", &show ) ) {
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_FittingPolicyResizeDown;
        if( ImGui::BeginTabBar( "Asset Types", tab_bar_flags ) ) {
            for( asset_library_cat &cat : assets.categories ) {
                show_assetlib_tab( cat, assets );
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

static void show_canvas_hint()
{
    ImGui::Text( "Use mouse to paint canvas with palette entries." );
}

static void show_palette_entry_extended( me_state &state, editor::me_palette &p,
        editor::me_palette_entry &entry )
{
    bool show = true;
    ImGui::Begin( "Extended Info", &show );

    if( ImGui::InputId( "ter", entry.ter ) ) {
        state.mark_changed();
        entry.sprite_cache_valid = false;
    }

    if( ImGui::InputId( "furn", entry.furn ) ) {
        state.mark_changed();
        entry.sprite_cache_valid = false;
    }

    ImGui::Text( "Pieces:" );

    cata::optional<size_t> del;
    cata::optional<size_t> move_up;
    cata::optional<size_t> move_dn;
    auto &list = entry.placing.pieces;
    for( size_t i = 0; i < list.size(); i++ ) {
        ImGui::PushID( i );
        ImGui::Separator();

        if( ImGui::ImageButton( "del", "me_delete" ) ) {
            del = i;
        }
        ImGui::HelpPopup( "Delete piece." );
        ImGui::SameLine();

        if( i == 0 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "up", ImGuiDir_Up ) ) {
            move_up = i;
        }
        ImGui::HelpPopup( "Move piece up." );
        if( i == 0 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if( i == list.size() - 1 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "down", ImGuiDir_Down ) ) {
            move_dn = i;
        }
        ImGui::HelpPopup( "Move piece down." );
        if( i == list.size() - 1 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        ImGui::Text( "Piece %d: %s", static_cast<int>( i ),
                     io::enum_to_string<PieceType>( list[i]->get_type() ).c_str() );

        list[i]->show_ui( state );

        ImGui::PopID();
    }
    if( del ) {
        list.erase( list.begin() + *del );
        state.mark_changed();
    }
    if( move_up ) {
        std::swap( list[*move_up], list[*move_up - 1] );
        state.mark_changed();
    }
    if( move_dn ) {
        std::swap( list[*move_dn], list[*move_dn + 1] );
        state.mark_changed();
    }

    static std::string new_piece_str;
    static std::vector<PieceType> piece_opts;
    if( piece_opts.empty() ) {
        new_piece_str += "Add piece...";
        new_piece_str += '\0';
        for( const auto &it : editor::get_piece_templates() ) {
            piece_opts.push_back( it->get_type() );
            new_piece_str += io::enum_to_string<PieceType>( it->get_type() );
            new_piece_str += '\0';
        }
    }

    int new_piece_type = 0;
    ImGui::Separator();
    if( ImGui::Combo( "##pick-new-piece", &new_piece_type, new_piece_str.c_str() ) ) {
        if( new_piece_type != 0 ) {
            list.push_back( editor::make_new_piece( piece_opts[new_piece_type - 1] ) );
            state.mark_changed();
        }
    }

    ImGui::End();
    if( !show ) {
        state.view_placings.reset();
    }
}

void show_file_info( me_state &state, me_file &file, bool &show )
{
    if( !ImGui::Begin( "File Info", &show ) ) {
        ImGui::End();
        return;
    }

    ImGui::Text( "Mapgen type:" );
    if( ImGui::RadioButton( "Oter", file.mtype == MapgenType::Oter ) ) {
        file.mtype = MapgenType::Oter;
        file.base.set_size( file.mapgensize().raw() );
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
    if( ImGui::RadioButton( "Update", file.mtype == MapgenType::Update ) ) {
        file.mtype = MapgenType::Update;
        file.base.set_size( file.mapgensize().raw() );
        state.mark_changed();
    }
    ImGui::HelpPopup(
        "Update mapgen.\n\n"
        "Invoked by basecamp upgrade routines.\n"
        "Can be used for automatic calculation of camp blueprint requirements."
    );
    ImGui::SameLine();
    if( ImGui::RadioButton( "Nested", file.mtype == MapgenType::Nested ) ) {
        file.mtype = MapgenType::Nested;
        file.base.set_size( file.mapgensize().raw() );
        state.mark_changed();
    }
    ImGui::HelpPopup(
        "Nested mapgen.\n\n"
        "Can be invoked by omt and upgrate mapgens.\n"
        "This is essentially a 'chunk' of any size up to 24x24 that can be procedurally placed by the calling mapgen."
    );
    ImGui::Separator();

    if( file.mtype == MapgenType::Oter ) {
        if( ImGui::InputId( "om_terrain", file.oter.om_terrain ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Overmap terrain type to assign this mapgen to." );
        if( ImGui::InputIntClamped( "weight", file.oter.weight, 0, 10000 ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Weight of this mapgen, defaults to 100.\n\n"
            "The higher this value is, the more frequently this mapgen will be chosen "
            "to generate the overmap terrain."
        );
        if( ImGui::InputIntRange( "rotation", file.oter.rotation ) ) {
            state.mark_changed();
        }
        ImGui::Text( "Oter mapgen base:" );
        ImGui::HelpPopup( "Defines how to fill in the 'empty' tiles in the canvas." );

        if( ImGui::RadioButton( "Fill terrain", file.oter.mapgen_base == OterMapgenBase::FillTer ) ) {
            file.oter.mapgen_base = OterMapgenBase::FillTer;
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Fill with terrain type.\n\n"
            "Useful for maps where most of the terrain is monotonic (e.g. solid rock, or open air)."
        );
        ImGui::SameLine();
        if( ImGui::RadioButton( "Predecessor mapgen",
                                file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) ) {
            file.oter.mapgen_base = OterMapgenBase::PredecessorMapgen;
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
        if( ImGui::RadioButton( "Rows", file.oter.mapgen_base == OterMapgenBase::Rows ) ) {
            file.oter.mapgen_base = OterMapgenBase::Rows;
            state.mark_changed();
        }
        ImGui::HelpPopup(
            "Use a 24x24 canvas to place tiles.\n\n"
            "The most straightforward method, just define a bunch of palettes ('symbol: data' pairs) "
            "and then place symbols on the canvas to define positions.\n"
            "Most useful for complex layouts with little variation, such as buildings."
        );

        if( file.oter.mapgen_base == OterMapgenBase::PredecessorMapgen ) {
            if( ImGui::InputId( "predecessor_mapgen", file.oter.predecessor_mapgen ) ) {
                state.mark_changed();
            }
            ImGui::HelpPopup( "Overmap type id to run predecessor mapgen for." );
        } else {
            if( ImGui::InputId( "fill_ter", file.oter.fill_ter ) ) {
                state.mark_changed();
            }
            ImGui::HelpPopup( "Terrain type to fill empty spots with." );
        }
        if( file.oter.mapgen_base == OterMapgenBase::Rows ) {
            show_canvas_hint();
        }
    } else if( file.mtype == MapgenType::Update ) {
        if( ImGui::InputText( "update_mapgen_id", &file.update.update_mapgen_id ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "ID of this update mapgen." );
        if( ImGui::InputId( "fill_ter", file.update.fill_ter ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Terrain type to fill empty spots with." );
    } else { // MapgenType::Nested
        if( ImGui::InputText( "nested_mapgen_id", &file.nested.nested_mapgen_id ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "ID of this nested mapgen." );
        if( ImGui::InputIntRange( "rotation", file.nested.rotation ) ) {
            state.mark_changed();
        }
        ImGui::HelpPopup( "Allowed rotations." );
        // Only square nested mapgens are possible
        if( ImGui::InputIntClamped( "mapgensize", file.nested.size.x, 1, SEEX * 2 ) ) {
            file.nested.size.y = file.nested.size.x;
            file.base.set_size( file.mapgensize().raw() );
            state.mark_changed();
        }
        ImGui::HelpPopup( "Size of this nested mapgen." );
        show_canvas_hint();
    }

    show_palette( state, file.base.inline_palette, state.show_base_inline_palette );

    ImGui::End();

    if( state.view_placings ) {
        editor::me_palette_entry *entry = file.base.inline_palette.find_entry( *state.view_placings );
        if( entry ) {
            show_palette_entry_extended( state, file.base.inline_palette, *entry );
        }
    }
}

static void show_palette_entries( me_state &state, std::vector<me_palette_entry> &list )
{
    std::unordered_set<map_key> checked;
    std::unordered_set<map_key> dupe_symbols;

    for( const editor::me_palette_entry &entry : list ) {
        if( checked.count( entry.key ) > 0 ) {
            dupe_symbols.insert( entry.key );
        } else {
            checked.insert( entry.key );
        }
    }

    cata::optional<size_t> del;
    cata::optional<size_t> move_up;
    cata::optional<size_t> move_dn;
    for( size_t i = 0; i < list.size(); i++ ) {
        ImGui::PushID( i );
        if( ImGui::ImageButton( "del", "me_delete" ) ) {
            del = i;
        }
        ImGui::HelpPopup( "Delete entry." );
        ImGui::SameLine();

        if( i == 0 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "up", ImGuiDir_Up ) ) {
            move_up = i;
        }
        ImGui::HelpPopup( "Move entry up." );
        if( i == 0 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        if( i == list.size() - 1 ) {
            ImGui::BeginDisabled();
        }
        if( ImGui::ArrowButton( "down", ImGuiDir_Down ) ) {
            move_dn = i;
        }
        ImGui::HelpPopup( "Move entry down." );
        if( i == list.size() - 1 ) {
            ImGui::EndDisabled();
        }
        ImGui::SameLine();

        uuid_t &brush = state.tools_state.brush;
        if( list[i].uuid == brush ) {
            if( ImGui::ImageButton( "unpick", "me_clear_rows_brush" ) ) {
                brush = UUID_INVALID;
            }
            ImGui::HelpPopup( "Unselect (turns brush into eraser)." );
        } else {
            if( ImGui::ImageButton( "pick", "me_set_rows_brush" ) ) {
                brush = list[i].uuid;
            }
            ImGui::HelpPopup( "Select as active for brush." );
        }
        ImGui::SameLine();

        if( ImGui::ColorEdit4( "MyColor##3", ( float * )&list[i].color,
                               ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel ) ) {
            state.mark_changed( "palette-entry-color" );
        }
        ImGui::SameLine();

        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() );
        bool is_dupe_symbol = dupe_symbols.count( list[i].key ) > 0;
        if( is_dupe_symbol ) {
            ImGui::BeginErrorArea();
        }
        if( ImGui::InputSymbol( "##key", list[i].key.str, default_map_key.str.c_str() ) ) {
            state.mark_changed( "palette-entry-key" );
        }
        ImGui::HelpPopup( "Symbol to use on canvas." );
        if( is_dupe_symbol ) {
            ImGui::EndErrorArea();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##furn", list[i].furn ) ) {
            state.mark_changed();
            list[i].sprite_cache_valid = false;
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth( ImGui::GetFrameHeight() * 15.0f );
        if( ImGui::InputId( "##ter", list[i].ter ) ) {
            state.mark_changed();
            list[i].sprite_cache_valid = false;
        }
        ImGui::SameLine();
        if( ImGui::ArrowButton( "##placing", ImGuiDir_Right ) ) {
            state.view_placings = list[i].uuid;
        }
        ImGui::PopID();
    }
    if( del ) {
        const uuid_t &uuid = list[ *del ].uuid;
        state.file().base.remove_usages( uuid );
        if( state.tools_state.brush == uuid ) {
            state.tools_state.brush = UUID_INVALID;
        }
        list.erase( list.begin() + *del );
        state.mark_changed();
    }
    if( move_up ) {
        std::swap( list[*move_up], list[*move_up - 1] );
        state.mark_changed();
    }
    if( move_dn ) {
        std::swap( list[*move_dn], list[*move_dn + 1] );
        state.mark_changed();
    }
    if( ImGui::ImageButton( "add", "me_add" ) ) {
        list.emplace_back( me_palette_entry{
            state.file().uuid_gen(),
            state.file().base.pick_available_key(),
            col_default_piece_color,
            false,
            cata::nullopt,
            ter_eid::NULL_ID(),
            furn_eid::NULL_ID(),
            me_placing()
        } );
        state.mark_changed();
    }
    ImGui::HelpPopup( "Add new entry." );
}

void show_palette( me_state &state, me_palette &p, bool &show )
{
    ImGui::PushID( &p );

    if( !ImGui::Begin( "Palette", &show ) ) {
        ImGui::End();
        ImGui::PopID();
        return;
    }

    if( p.is_inline ) {
        ImGui::Text( "<inline palette>" );
    } else {
        ImGui::Text( "id: %s", p.id.data.c_str() );
    }

    show_palette_entries( state, p.entries );

    ImGui::End();
    ImGui::PopID();
}

void show_toolbar( me_state &state, bool &show )
{
    if( !ImGui::Begin( "Toolbar", &show,
                       ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoCollapse |
                       ImGuiWindowFlags_NoResize
                     ) ) {
        ImGui::End();
        return;
    }

    me_canvas_tools_state &tools = state.tools_state;

    if( ImGui::RadioButton( "Brush", tools.tool == CanvasTool::Brush ) ) {
        tools.tool = CanvasTool::Brush;
    }
    ImGui::HelpPopup( "Hold LMB to draw with selected tile." );
    if( ImGui::RadioButton( "Bucket", tools.tool == CanvasTool::Bucket ) ) {
        tools.tool = CanvasTool::Bucket;
    }
    ImGui::HelpPopup( "Click LMB to flood fill with selected tile." );
    if( ImGui::RadioButton( "Bucket (global)", tools.tool == CanvasTool::BucketGlobal ) ) {
        tools.tool = CanvasTool::BucketGlobal;
    }
    ImGui::HelpPopup( "Click LMB to replace all such tiles with selected tile." );

    ImGui::End();
}

static void handle_revision_change( me_state &state )
{
    if( state.tools_state.ongoing_tool_operation ) {
        return;
    }
    if( ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) && ImGui::IsKeyPressed( ImGuiKey_Z ) ) {
        if( ImGui::IsKeyDown( ImGuiKey_LeftShift ) ) {
            if( state.can_redo() ) {
                state.queue_redo();
            }
        } else {
            if( state.can_undo() ) {
                state.queue_undo();
            }
        }
    }
    if( state.switch_to_revision ) {
        auto it = std::find_if( state.file_history.cbegin(),
        state.file_history.cend(), [&]( const me_file_revision & rev ) {
            return rev.num == *state.switch_to_revision;
        } );
        assert( it != state.file_history.cend() );
        state.current_revision = it->make_copy();
        state.switch_to_revision.reset();
    } else if( state.file_has_changes ) {
        state.file_has_changes = false;

        const bool is_changing_same = state.last_widget_changed && state.current_widget_changed &&
                                      *state.last_widget_changed == *state.current_widget_changed;

        state.current_widget_changed_str.clear();
        state.last_widget_changed = state.current_widget_changed;
        state.current_widget_changed = cata::nullopt;

        bool is_alt_history = false;

        // Erase alternative history
        while( state.file_history[0].num != state.current_revision.num ) {
            // TODO: optimize this to use dequeue
            state.file_history.erase( state.file_history.cbegin() );
            is_alt_history = true;
        }

        const bool is_rev_saved = state.last_saved_revision ? *state.last_saved_revision ==
                                  state.current_revision.num : false;
        const bool is_rev_exported = state.last_exported_revision ? *state.last_exported_revision ==
                                     state.current_revision.num : false;
        const bool collapse_change = is_changing_same && !is_alt_history && !is_rev_saved &&
                                     !is_rev_exported && !state.file_history.empty();

        if( collapse_change ) {
            state.file_history.erase( state.file_history.cbegin() );
        } else {
            state.current_revision.num++;
        }
        state.file_history.insert( state.file_history.cbegin(), state.current_revision.make_copy() );

        // Erase old entries
        if( static_cast<int>( state.file_history.size() ) > state.history_capacity ) {
            state.file_history.resize( state.history_capacity );
        }
    }
}

void show_me_ui( me_state &state )
{
    show_canvas( state );
    show_control_window( state );
    if( state.show_demo_wnd ) {
        ImGui::ShowDemoWindow( &state.show_demo_wnd );
    }
    if( state.show_asset_lib ) {
        show_asset_lib( state.assets, state.show_asset_lib );
    }
    if( state.show_file_info ) {
        show_file_info( state, state.file(), state.show_file_info );
    }
    if( state.show_file_history ) {
        show_file_history( state, state.show_file_history );
    }
    if( state.show_toolbar ) {
        show_toolbar( state, state.show_toolbar );
    }

    handle_revision_change( state );
}

point_rel_etile me_file::mapgensize() const
{
    if( mtype == MapgenType::Nested ) {
        return point_rel_etile( nested.size );
    } else {
        return point_rel_etile( SEEX * 2, SEEY * 2 );
    }
}

me_map_key_generator::me_map_key_generator()
{
    const translation &trans = SNIPPET.get_snippet_ref_by_id( snippet_id( "me_auto_map_keys" ) );
    std::u32string s_u32 = utf8_to_utf32( trans.raw );
    // TODO: support combining characters
    opts.reserve( s_u32.size() );
    for( const char32_t &ch32 : s_u32 ) {
        opts.emplace_back( utf32_to_utf8( ch32 ) );
    }
}

void me_map_key_generator::blacklist( const map_key &opt )
{
    std::remove( opts.begin(), opts.end(), opt );
}

me_placing::me_placing( const me_placing &rhs )
{
    *this = rhs;
}

me_placing &me_placing::operator=( const me_placing &rhs )
{
    pieces.reserve( rhs.pieces.size() );
    for( const auto &piece : rhs.pieces ) {
        pieces.emplace_back( piece->clone() );
    }
    return *this;
}

me_state::me_state() : me_state( std::make_unique<me_file>() ) { }

me_state::me_state( std::unique_ptr<me_file> &&file ) : me_state( std::move( file ), nullptr ) { }

me_state::me_state( std::unique_ptr<me_file> &&file,
                    const std::string *loaded_from_path )
{
    current_revision = me_file_revision();

    if( loaded_from_path ) {
        file_save_path = *loaded_from_path;
        last_saved_revision = current_revision.num;
    }

    if( file ) {
        current_revision.file = std::move( file );
    }

    file_history.reserve( history_capacity + 1 );
    file_history.emplace_back( current_revision.make_copy() );

    init_assets( assets );
}

me_state::~me_state() = default;

void me_state::mark_changed( const char *id )
{
    std::string new_widget_changed_str = id ? id : "<nullptr>";
    if( file_has_changes ) {
        std::cerr << string_format(
                      "Tried to invoke mark_changed( \"%s\" ), but the file has already been marked as changed with id \"%s\".",
                      new_widget_changed_str,
                      current_widget_changed_str
                  ) << std::endl;
        std::abort();
    }
    current_widget_changed_str = new_widget_changed_str;
    if( id ) {
        ImGuiWindow *wnd = ImGui::GetCurrentWindow();
        current_widget_changed = wnd->GetID( id );
    }
    file_has_changes = true;
    edit_counter++;
}

bool me_state::has_unsaved_changes() const
{
    return !last_saved_revision || current_revision.num != *last_saved_revision;
}

bool me_state::has_unexported_changes() const
{
    return !last_exported_revision || current_revision.num != *last_exported_revision;
}

const map_key &me_palette::key_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return default_map_key;
    }
    const me_palette_entry *entry = find_entry( uuid );
    if( entry ) {
        return entry->key;
    }

    std::cerr << "Tried to find palette key, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const ImVec4 &me_palette::color_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        static ImVec4 default_color = ImVec4();
        return default_color;
    }
    const me_palette_entry *entry = find_entry( uuid );
    if( entry ) {
        return entry->color;
    }

    std::cerr << "Tried to find palette color, but uuid was not found " << uuid << std::endl;
    std::abort();
}

const SpriteRef *me_palette::sprite_from_uuid( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    const me_palette_entry *entry = find_entry( uuid );
    if( entry ) {
        if( !entry->sprite_cache_valid ) {
            entry->build_sprite_cache();
        }
        if( !entry->sprite_cache ) {
            return nullptr;
        } else {
            return &*entry->sprite_cache;
        }
    }

    std::cerr << "Tried to find sprite, but uuid was not found " << uuid << std::endl;
    std::abort();
}

me_palette_entry *me_palette::find_entry( const uuid_t &uuid )
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    for( auto &it : entries ) {
        if( it.uuid == uuid ) {
            return &it;
        }
    }
    return nullptr;
}

const me_palette_entry *me_palette::find_entry( const uuid_t &uuid ) const
{
    if( uuid == UUID_INVALID ) {
        return nullptr;
    }
    for( const auto &it : entries ) {
        if( it.uuid == uuid ) {
            return &it;
        }
    }
    return nullptr;
}

void me_palette_entry::build_sprite_cache() const
{
    if( !furn.is_null() && furn.is_valid() ) {
        sprite_cache = SpriteRef( furn.data );
    } else if( !ter.is_null() && ter.is_valid() ) {
        sprite_cache = SpriteRef( ter.data );
    } else {
        sprite_cache.reset();
    }
    sprite_cache_valid = true;
}

void me_mapgen_base::set_size( const point &s )
{
    if( size == s ) {
        return;
    }
    // TODO: graciously transfer entries from old size
    size = s;
    rows.clear();
    rows.resize( s.x * s.y, UUID_INVALID );
}

me_mapgen_base::~me_mapgen_base() = default;

map_key me_mapgen_base::pick_available_key() const
{
    me_map_key_generator gen;
    for( const auto &it : inline_palette.entries ) {
        gen.blacklist( it.key );
    }
    return gen();
}

void me_mapgen_base::remove_usages( const uuid_t &uuid )
{
    for( uuid_t &cell : rows ) {
        if( cell == uuid ) {
            cell = UUID_INVALID;
        }
    }
}

} // namespace editor
