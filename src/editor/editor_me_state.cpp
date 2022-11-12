#include "editor_me_state.h"
#include "editor_widgets.h"
#include "editor_me_state_export.h"
#include "editor_me_canvas.h"

#include "../fstream_utils.h"
#include "../game_constants.h"
#include "../game.h"
#include "../string_utils.h"
#include "../text_snippets.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <unordered_set>

namespace editor
{

void show_me_ui( me_state &state )
{
    run_ui_for_state( state );
}

me_state::me_state() : me_state( std::make_unique<me_file>() ) { }

me_state::me_state( std::unique_ptr<me_file> &&file ) : me_state( std::move( file ), nullptr ) { }

me_state::me_state( std::unique_ptr<me_file> &&file,
                    const std::string *loaded_from_path ) : histate( std::move( file ), !!loaded_from_path )
{
    if( loaded_from_path ) {
        sestate.file_save_path = *loaded_from_path;
    }
    init_assets( assets );
}

me_state::~me_state() = default;

} // namespace editor
