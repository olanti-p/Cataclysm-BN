#include "selection_mask.h"

namespace editor
{

void SelectionMask::clear_all()
{
    data.set_all( false );
}

void SelectionMask::set_all()
{
    data.set_all( true );
}

} // namespace editor
