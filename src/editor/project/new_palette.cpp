#include "new_palette.h"

#include "imgui.h"
#include "mapgen/palette.h"
#include "state/state.h"
#include "common/uuid.h"
#include "widget/widgets.h"
#include "state/ui_state.h"
#include "project.h"

#include <chrono>
#include <cstddef>

namespace editor
{
bool show_new_palette_window( State &state, NewPaletteState &palette )
{
    bool keep_open = true;
    ImGui::Begin( "New Palette", &keep_open );
    if( !keep_open ) {
        palette.cancelled = true;
    }

    ImGui::Checkbox( "Create inheriting palette", &palette.inherits );
    ImGui::BeginDisabled( !palette.inherits );
    ImGui::PaletteSelector( "Inherit from", palette.inherits_from, state.project().palettes );
    ImGui::EndDisabled();

    ImGui::Separator();

    ImGui::InputText( "Name", &palette.name );
    ImGui::HelpPopup( "Display name.  Has no effect, just for convenience." );

    bool input_ok = true;
    if( palette.inherits && !state.project().get_palette( palette.inherits_from ) ) {
        input_ok = false;
    }

    ImGui::BeginDisabled( !input_ok );
    if( ImGui::Button( "Confirm" ) ) {
        palette.confirmed = true;
    }
    ImGui::EndDisabled();

    ImGui::End();

    if( palette.cancelled ) {
        return false;
    }
    if( palette.confirmed ) {
        add_palette( state, palette );
        state.mark_changed();
        return false;
    }
    return true;
}

void add_palette( State &state, NewPaletteState &palette )
{
    Project &project = state.project();
    UUID new_palette_uuid = project.uuid_generator();
    project.palettes.emplace_back();
    Palette &new_palette = project.palettes.back();
    new_palette.uuid = new_palette_uuid;
    new_palette.name = palette.name;

    if( palette.inherits ) {
        new_palette.inherits_from = palette.inherits_from;
    }
}

} // namespace editor
