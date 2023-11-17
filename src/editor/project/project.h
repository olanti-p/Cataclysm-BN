#ifndef CATA_SRC_EDITOR_PROJECT_H
#define CATA_SRC_EDITOR_PROJECT_H

#include "mapgen/mapgen.h"
#include "mapgen/palette.h"
#include "common/uuid.h"

namespace editor
{
struct State;

struct Project {
    std::string project_uuid;
    UUIDGenerator uuid_gen;
    std::vector<Mapgen> files;
    std::vector<Palette> palettes;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );

    const Mapgen *get_file_by_uuid( const UUID &fid ) const;
    inline Mapgen *get_file_by_uuid( const UUID &fid ) {
        const Project *this_c = this;
        return const_cast<Mapgen *>( this_c->get_file_by_uuid( fid ) );
    }

    const Palette *get_palette_by_uuid( const UUID &pid ) const;
    inline Palette *get_palette_by_uuid( const UUID &pid ) {
        const Project *this_c = this;
        return const_cast<Palette *>( this_c->get_palette_by_uuid( pid ) );
    }
};

void show_project_overview_ui( State &state, Project &project, bool &show );

std::unique_ptr<Project> create_empty_project();

} // namespace editor

#endif // CATA_SRC_EDITOR_PROJECT_H
