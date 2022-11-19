#ifndef CATA_SRC_EDITOR_EDITOR_ME_MAPOBJECT_H
#define CATA_SRC_EDITOR_EDITOR_ME_MAPOBJECT_H

#include "editor_me_int_range.h"
#include "editor_me_uuid.h"

#include <memory>

namespace editor
{
struct me_file;
struct me_piece;
struct me_state;

struct me_mapobject {
    me_mapobject();
    me_mapobject( const me_mapobject &rhs );
    me_mapobject( me_mapobject && );
    ~me_mapobject();

    me_mapobject &operator=( const me_mapobject &rhs );
    me_mapobject &operator=( me_mapobject && );

    const uuid_t &get_uuid() const;
    void set_uuid( const uuid_t &uuid );

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    std::unique_ptr<me_piece> piece;
    me_int_range x;
    me_int_range y;
    me_int_range repeat;
};

/**
 * =============== Windows ===============
 */
void show_mapobjects( me_state &state, me_file &f, bool &show );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_MAPOBJECT_H
