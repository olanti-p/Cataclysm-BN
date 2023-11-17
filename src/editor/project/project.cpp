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
        UUID this_uuid = project.mapgens[idx].uuid;
        if( ImGui::ImageButton( "toggle_palette", "me_palette" ) ) {
            state.ui->toggle_show_palette( project.mapgens[idx].base.inline_palette_id );
        }
        ImGui::HelpPopup( "Show/hide inline palette for this mapgen." );
        ImGui::SameLine();
        if( ImGui::ImageButton( "toggle_mapobjects", "me_mapobject" ) ) {
            state.ui->toggle_show_mapobjects( project.mapgens[idx].uuid );
        }
        ImGui::HelpPopup( "Show/hide map objects for this mapgen." );
        ImGui::SameLine();
        if( ImGui::Selectable(
                string_format( "Mapgen #%d", idx ).c_str(),
                state.ui->active_mapgen_id && *state.ui->active_mapgen_id == this_uuid )
          ) {
            state.ui->active_mapgen_id = this_uuid;
        }
    } )
    .with_add( [&]()->bool {
        bool ret = false;
        if( ImGui::Button( "New mapgen" ) )
        {
            UUID new_mapgen = project.uuid_generator();
            project.mapgens.emplace_back();
            project.mapgens.back().uuid = new_mapgen;
            UUID new_palette = project.uuid_generator();
            project.mapgens.back().base.inline_palette_id = new_palette;
            project.palettes.emplace_back();
            project.palettes.back().uuid = new_palette;
            ret = true;
        }
        return ret;
    } )
    .with_delete( [&]( size_t idx ) {
        UUID pal_uuid = project.mapgens[idx].base.inline_palette_id;
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
        Palette pcopy = *project.get_palette( copy.base.inline_palette_id );
        copy.uuid = project.uuid_generator();
        pcopy.uuid = project.uuid_generator();
        copy.base.inline_palette_id = pcopy.uuid;
        project.mapgens.insert( std::next( project.mapgens.cbegin(), idx + 1 ), std::move( copy ) );
        project.palettes.push_back( std::move( pcopy ) );
    } )
    .run( project.mapgens );

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
