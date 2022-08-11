#ifndef CATA_SRC_EDITOR_EDITOR_ASSETS_H
#define CATA_SRC_EDITOR_EDITOR_ASSETS_H

#include <string>

#include "../type_id.h"

struct ter_t;
struct furn_t;
struct trap;
struct field_type;
struct itype;
struct mtype;
struct MonsterGroup;
class mapgen_palette;
class mapgen_function;
class update_mapgen_function_json;
class mapgen_function_json_nested;

namespace editor
{
// For some fun reason item_group_id does not actually refer to any item group.
// Not that item groups appear to be stored at all...!?
struct igroup_plug {
    item_group_id id;
};

struct nested_mapgen_plug {
    std::string id;
    mapgen_function_json_nested *data = nullptr;
};

struct update_mapgen_plug {
    std::string id;
    update_mapgen_function_json *data = nullptr;
};

struct oter_mapgen_plug {
    std::string id;
    mapgen_function *data = nullptr;
};

enum class AssetType : int {
    Terrain = 0,
    Furniture,
    Trap,
    Field,
    Itype,
    Igroup,
    Mtype,
    Mgroup,
    Palette,
    NestedMapgen,
    UpdateMapgen,
    OterMapgen,

    NumAssetTypes
};

const char *get_asset_type_name( AssetType at );

class asset_lib_entry
{
    public:
        asset_lib_entry() = default;
        virtual ~asset_lib_entry() = default;

        virtual AssetType get_type() const = 0;
        virtual const char *get_id() const = 0;
        virtual void show_details() const {};
};

class asset_terrain : public asset_lib_entry
{
    public:
        const ter_t &ref;

        asset_terrain( const ter_t &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Terrain;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_furniture : public asset_lib_entry
{
    public:
        const furn_t &ref;

        asset_furniture( const furn_t &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Furniture;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_trap : public asset_lib_entry
{
    public:
        const trap &ref;

        asset_trap( const trap &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Trap;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_field_type : public asset_lib_entry
{
    public:
        const field_type &ref;

        asset_field_type( const field_type &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Field;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_itype : public asset_lib_entry
{
    public:
        const itype &ref;

        asset_itype( const itype &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Itype;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_igroup : public asset_lib_entry
{
    public:
        const igroup_plug &ref;

        asset_igroup( const igroup_plug &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Igroup;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_mtype : public asset_lib_entry
{
    public:
        const mtype &ref;

        asset_mtype( const mtype &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Mtype;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_mgroup : public asset_lib_entry
{
    public:
        const MonsterGroup &ref;

        asset_mgroup( const MonsterGroup &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Mgroup;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_palette : public asset_lib_entry
{
    public:
        const mapgen_palette &ref;

        asset_palette( const mapgen_palette &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::Palette;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_nested_mapgen : public asset_lib_entry
{
    public:
        const nested_mapgen_plug &ref;

        asset_nested_mapgen( const nested_mapgen_plug &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::NestedMapgen;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_update_mapgen : public asset_lib_entry
{
    public:
        const update_mapgen_plug &ref;

        asset_update_mapgen( const update_mapgen_plug &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::UpdateMapgen;
        }
        const char *get_id() const override;
        void show_details() const override;
};

class asset_oter_mapgen : public asset_lib_entry
{
    public:
        const oter_mapgen_plug &ref;

        asset_oter_mapgen( const oter_mapgen_plug &r ) : ref( r ) {}
        AssetType get_type() const override {
            return AssetType::OterMapgen;
        }
        const char *get_id() const override;
        void show_details() const override;
};

struct asset_library_cat {
    private:
        std::vector<std::unique_ptr<asset_lib_entry>> entries;

    public:
        std::string filter;
        AssetType atype = AssetType::NumAssetTypes;
        int selected = 0;
        bool is_active_tab = false;

        void add_asset( std::unique_ptr<asset_lib_entry> &&e ) {
            entries.push_back( std::move( e ) );
        }

        int get_num() const {
            return entries.size();
        }

        const asset_lib_entry &get( int i ) const {
            return *entries[i];
        }
};

struct asset_library {
    std::vector<igroup_plug> igroup_plugs;
    std::vector<nested_mapgen_plug> nested_mapgen_plugs;
    std::vector<update_mapgen_plug> update_mapgen_plugs;
    std::vector<oter_mapgen_plug> oter_mapgen_plugs;

    std::vector<asset_library_cat> categories;

    asset_library_cat &add_cat( AssetType at ) {
        asset_library_cat new_cat;
        new_cat.atype = at;
        if( categories.empty() ) {
            // A single tab must be active
            new_cat.is_active_tab = true;
        }
        categories.push_back( std::move( new_cat ) );
        return categories.back();
    }

    asset_library_cat &get_cat( AssetType at ) {
        for( asset_library_cat &it : categories ) {
            if( at == it.atype ) {
                return it;
            }
        }
        std::abort();
    }

    const asset_library_cat &get_cat( AssetType at ) const {
        for( const asset_library_cat &it : categories ) {
            if( at == it.atype ) {
                return it;
            }
        }
        std::abort();
    }

    template<typename T, typename R>
    void add_asset( const R &raw ) {
        std::unique_ptr<asset_lib_entry> ptr = std::make_unique<T>( raw );
        get_cat( ptr->get_type() ).add_asset( std::move( ptr ) );
    }

    const asset_lib_entry &get_selected_asset() const {
        for( const asset_library_cat &cat : categories ) {
            if( cat.is_active_tab ) {
                return cat.get( cat.selected );
            }
        }
        std::abort();
    }
};

void init_assets( asset_library &assets );

} // namespace editor

#endif // CATA_SRC_EDITOR_EDITOR_ASSETS_H
