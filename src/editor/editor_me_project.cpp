#include "editor_me_project.h"

#include "editor_me_file.h"
#include "editor_me_state.h"
#include "editor_widgets.h"
#include "editor_me_uistate.h"

namespace editor
{

me_file *me_project::get_file_by_uuid( const uuid_t &fid )
{
    for( me_file &file : files ) {
        if( fid == file.uuid ) {
            return &file;
        }
    }
    return nullptr;
}

void show_project_ui( me_state &state, me_project &project )
{
    ImGui::Begin( "Project Overview" );

    ImGui::Text( "Mapgens:" );

    bool changed = ImGui::VectorWidget()
    .with_for_each( [&]( size_t idx ) {
        uuid_t this_uuid = project.files[idx].uuid;
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
            uuid_t new_uuid = project.uuid_gen();
            project.files.emplace_back();
            project.files.back().uuid = new_uuid;
            ret = true;
        }
        return ret;
    } )
    .with_duplicate( [&]( size_t idx ) {
        me_file copy = project.files[ idx ];
        copy.uuid = project.uuid_gen();
        project.files.insert( std::next( project.files.cbegin(), idx + 1 ), std::move( copy ) );
    } )
    .run( project.files );

    if( changed ) {
        state.mark_changed();
    }

    ImGui::End();
}

} // namespace editor
