#include "editor_me_piece.h"

#include "editor_me_state.h"
#include "editor_widgets.h"

#define REG_PIECE( piece_class ) ret.push_back( std::make_unique<piece_class>() )

namespace editor
{

void me_piece_field::show_ui( me_state &state )
{
    if( ImGui::InputId( "ftype", ftype ) ) {
        state.mark_changed();
    }
    if( ImGui::InputIntClamped( "intensity", intensity, 1, 3 ) ) {
        state.mark_changed();
    }
    if( ImGui::InputDuration( "age", age ) ) {
        state.mark_changed();
    }
}

const std::vector<std::unique_ptr<me_piece>> &get_piece_templates()
{
    static std::vector<std::unique_ptr<me_piece>> ret;
    if( ret.empty() ) {
        ret.reserve( static_cast<int>( PieceType::NumJmTypes ) );
        REG_PIECE( me_piece_field );
    }
    return ret;
}

std::unique_ptr<me_piece> make_new_piece( PieceType pt )
{
    for( const std::unique_ptr<me_piece> &it : get_piece_templates() ) {
        if( it->get_type() == pt ) {
            return it->clone();
        }
    }
    std::cerr << "Failed to generate piece with type " << static_cast<int>( pt ) << std::endl;
    std::abort();
}

} // namespace editor
