#ifndef CATA_SRC_EDITOR_EDITOR_ME_PROJECT_H
#define CATA_SRC_EDITOR_EDITOR_ME_PROJECT_H

#include "editor_me_file.h"

namespace editor
{
struct me_state;

struct me_project {
    uuid_generator uuid_gen;
    std::vector<me_file> files;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    me_file *get_file_by_uuid( const uuid_t &fid );
};

void show_project_ui( me_state &state, me_project &project );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ME_PROJECT_H
