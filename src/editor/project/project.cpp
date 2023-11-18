#include "project.h"

#include "common/uuid.h"
#include "imgui.h"
#include "mapgen/mapgen.h"
#include "mapgen/palette.h"
#include "new_mapgen.h"
#include "state/state.h"
#include "state/ui_state.h"
#include "widget/widgets.h"

#include <chrono>
#include <cstddef>
#include <memory>

namespace editor
{

const Mapgen *Project::get_mapgen( const UUID &fid ) const
{
    for( const Mapgen &mapgen : mapgens ) {
        if( fid == mapgen.uuid ) {
            return &mapgen;
        }
    }
    return nullptr;
}

const Palette *Project::get_palette( const UUID &fid ) const
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
        Mapgen &mapgen = project.mapgens[idx];
        if( ImGui::ImageButton( "toggle_palette", "me_palette" ) ) {
            state.ui->toggle_show_palette( mapgen.base.palette );
        }
        ImGui::HelpPopup( "Show/hide inline palette for this mapgen." );
        ImGui::SameLine();
        if( ImGui::ImageButton( "toggle_mapobjects", "me_mapobject" ) ) {
            state.ui->toggle_show_mapobjects( mapgen.uuid );
        }
        ImGui::HelpPopup( "Show/hide map objects for this mapgen." );
        ImGui::SameLine();
        if( ImGui::Selectable(
                mapgen.display_name().c_str(),
                state.ui->active_mapgen_id && *state.ui->active_mapgen_id == mapgen.uuid )
          ) {
            state.ui->active_mapgen_id = mapgen.uuid;
        }
    } )
    .with_add( [&]()->bool {
        if( ImGui::Button( "New mapgen" ) )
        {
            state.ui->new_mapgen_window = std::make_unique<NewMapgenState>();
        }
        return false;
    } )
    .with_delete( [&]( size_t idx ) {
        UUID pal_uuid = project.mapgens[idx].base.palette;
        project.mapgens.erase( std::next( project.mapgens.cbegin(), idx ) );
        for( auto it = project.palettes.cbegin(); it != project.palettes.cend(); it++ ) {
            if( it->uuid == pal_uuid ) {
                project.palettes.erase( it );
                break;
            }
        }
    } )
    .with_duplicate( [&]( size_t idx ) {
        Mapgen copy = project.mapgens[ idx ];
        Palette pcopy = *project.get_palette( copy.base.palette );
        copy.uuid = project.uuid_generator();
        pcopy.uuid = project.uuid_generator();
        copy.base.palette = pcopy.uuid;
        project.mapgens.insert( std::next( project.mapgens.cbegin(), idx + 1 ), std::move( copy ) );
        project.palettes.push_back( std::move( pcopy ) );
    } )
    .run( project.mapgens );

    ImGui::Text( "Inline palettes:" );
    for( const Palette &pal : project.palettes ) {
        ImGui::Selectable( pal.display_name().c_str(), false );
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
