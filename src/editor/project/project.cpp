#include "project.h"

#include "mapgen/mapgen.h"
#include "mapgen/palette.h"
#include "state/state.h"
#include "common/uuid.h"
#include "widget/widgets.h"
#include "state/ui_state.h"

#include <chrono>

namespace editor
{

const Mapgen *Project::get_file_by_uuid( const UUID &fid ) const
{
    for( const Mapgen &file : files ) {
        if( fid == file.uuid ) {
            return &file;
        }
    }
    return nullptr;
}

const Palette *Project::get_palette_by_uuid( const UUID &fid ) const
{
    for( const Palette &palette : palettes ) {
        if( fid == palette.uuid ) {
            return &palette;
        }
    }
    return nullptr;
}

void show_project_overview_ui( State &state, Project &project, bool &show )
{
    ImGui::SetNextWindowSize( ImVec2( 250.0f, 200.0f ), ImGuiCond_FirstUseEver );
    ImGui::Begin( "Project Overview", &show );

    ImGui::Text( "Mapgens:" );

    bool changed_mapgens = ImGui::VectorWidget()
    .with_for_each( [&]( size_t idx ) {
        UUID this_uuid = project.files[idx].uuid;
        if( ImGui::ImageButton( "toggle_palette", "me_palette" ) ) {
            state.uistate->toggle_show_palette( project.files[idx].base.inline_palette_id );
        }
        ImGui::HelpPopup( "Show/hide inline palette for this mapgen." );
        ImGui::SameLine();
        if( ImGui::ImageButton( "toggle_mapobjects", "me_mapobject" ) ) {
            state.uistate->toggle_show_mapobjects( project.files[idx].uuid );
        }
        ImGui::HelpPopup( "Show/hide map objects for this mapgen." );
        ImGui::SameLine();
        if( ImGui::Selectable(
                string_format( "Mapgen #%d", idx ).c_str(),
                state.uistate->active_file_id && *state.uistate->active_file_id == this_uuid )
          ) {
            state.uistate->active_file_id = this_uuid;
        }
    } )
    .with_add( [&]()->bool {
        bool ret = false;
        if( ImGui::Button( "New mapgen" ) )
        {
            UUID new_mapgen = project.uuid_gen();
            project.files.emplace_back();
            project.files.back().uuid = new_mapgen;
            UUID new_palette = project.uuid_gen();
            project.files.back().base.inline_palette_id = new_palette;
            project.palettes.emplace_back();
            project.palettes.back().uuid = new_palette;
            ret = true;
        }
        return ret;
    } )
    .with_delete( [&]( size_t idx ) {
        UUID pal_uuid = project.files[idx].base.inline_palette_id;
        project.files.erase( std::next( project.files.cbegin(), idx ) );
        for( auto it = project.palettes.cbegin(); it != project.palettes.cend(); it++ ) {
            if( it->uuid == pal_uuid ) {
                project.palettes.erase( it );
                break;
            }
        }
    } )
    .with_duplicate( [&]( size_t idx ) {
        Mapgen copy = project.files[ idx ];
        Palette pcopy = *project.get_palette_by_uuid( copy.base.inline_palette_id );
        copy.uuid = project.uuid_gen();
        pcopy.uuid = project.uuid_gen();
        copy.base.inline_palette_id = pcopy.uuid;
        project.files.insert( std::next( project.files.cbegin(), idx + 1 ), std::move( copy ) );
        project.palettes.push_back( std::move( pcopy ) );
    } )
    .run( project.files );

    ImGui::Text( "Inline palettes:" );
    for( const Palette &pal : project.palettes ) {
        ImGui::Selectable( string_format( "Palette [uuid=%d]", pal.uuid ).c_str(), false );
    }

    if( changed_mapgens ) {
        state.mark_changed();
    }

    ImGui::End();
}

std::string timestamp_string()
{
    using namespace std::chrono;
    uint64_t ms = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
    return string_format( "%u", ms );
}

std::unique_ptr<Project> create_empty_project()
{
    auto ret = std::make_unique<Project>();
    ret->project_uuid = timestamp_string();
    return ret;
}

} // namespace editor
