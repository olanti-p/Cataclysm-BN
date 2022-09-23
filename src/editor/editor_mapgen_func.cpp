#include "editor_assets.h"
#include "editor_main.h"
#include "editor_widgets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include "../mapgen.h"
#include "../mapgen_piece.h"

static void JmapgenPieceWidget( const jmapgen_piece &jmp )
{
    ImGui::Text( "Type: %s", io::enum_to_string<JmPieceType>( jmp.get_type() ).c_str() );
    ImGui::JmapgenInt( "repeat", jmp.repeat );
    jmp.show_details();
}

void mapgen_function::editor_show_details() const
{
    ImGui::Separator();
    ImGui::Text( "MAPGEN_FUNCTION" );
    ImGui::Text( "weight: %d", weight );
}

void mapgen_function_builtin::editor_show_details() const
{
    mapgen_function::editor_show_details();
    ImGui::Separator();
    ImGui::Text( "BUILTIN" );
    ImGui::Text( "function: %s", fname.c_str() );
}

void mapgen_function_json_base::editor_show_details_base() const
{
    ImGui::Separator();
    ImGui::Text( "JSON_BASE" );
    ImGui::Text( "size: %s", mapgensize.to_string().c_str() );
    ImGui::Text( "offset: %s", m_offset.to_string().c_str() );
    ImGui::Text( "setmap: %d elem(s)", static_cast<int>( setmap_points.size() ) );
    ImGui::Text( "objects size: %s", objects.mapgensize.to_string().c_str() );
    ImGui::Text( "objects offset: %s", objects.m_offset.to_string().c_str() );

    int num_objects = static_cast<int>( objects.objects.size() );
    if( ImGui::TreeNode( "objects_list", "objects data: %d elem(s)", num_objects ) ) {
        for( int i = 0; i < num_objects; i++ ) {
            const void *node_id = ( const void * )( intptr_t )i;
            if( ImGui::TreeNode( node_id, "elem %d", i ) ) {
                const jmapgen_objects::jmapgen_obj &ref = objects.objects[i];
                ImGui::Separator();
                ImGui::JmapgenPlace( "placement", ref.first );
                ImGui::Separator();
                JmapgenPieceWidget( *ref.second );
                ImGui::Separator();

                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
}

void mapgen_function_json::editor_show_details() const
{
    mapgen_function_json_base::editor_show_details_base();
    mapgen_function::editor_show_details();
    ImGui::Separator();
    ImGui::Text( "OTER_MAPGEN" );
    ImGui::SameLine();
    if( ImGui::Button( "Set as active" ) ) {
        editor::set_as_active( this );
    }
    ImGui::Text( "fill_ter: %s", fill_ter.id().c_str() );
    ImGui::Text( "predecessor_mapgen: %s", predecessor_mapgen.id().c_str() );
    ImGui::JmapgenInt( "rotation", rotation );
}

void update_mapgen_function_json::editor_show_details() const
{
    mapgen_function_json_base::editor_show_details_base();
    ImGui::Separator();
    ImGui::Text( "UPDATE_MAPGEN" );
    ImGui::Text( "fill_ter: %s", fill_ter.id().c_str() );
}

void mapgen_function_json_nested::editor_show_details() const
{
    mapgen_function_json_base::editor_show_details_base();
    ImGui::Separator();
    ImGui::Text( "NESTED_MAPGEN" );
    ImGui::JmapgenInt( "rotation", rotation );
}
