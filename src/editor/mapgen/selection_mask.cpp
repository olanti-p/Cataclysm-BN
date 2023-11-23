#include "selection_mask.h"

namespace editor
{

void SelectionMask::clear_all()
{
    data.set_all( Bool( false ) );
}

void SelectionMask::set_all()
{
    data.set_all( Bool( true ) );
}

} // namespace editor
