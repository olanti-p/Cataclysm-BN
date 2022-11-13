#include "editor_me_state.h"

#include "editor_me_assetlib.h"
#include "editor_me_camera.h"
#include "editor_me_canvas_tool.h"
#include "editor_me_file.h"
#include "editor_me_history.h"
#include "editor_me_save_export.h"
#include "editor_me_uistate.h"

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
        sestate->file_save_path = *loaded_from_path;
    }
    init_assets( *assets );
}

me_state::~me_state() = default;

me_state::me_state( me_state && ) = default;
me_state &me_state::operator=( me_state && ) = default;

me_file &me_state::file()
{
    return histate->file();
}

void me_state::mark_changed( const char *id )
{
    histate->mark_changed( id );
}

} // namespace editor
