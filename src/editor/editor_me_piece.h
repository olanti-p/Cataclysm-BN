#ifndef CATA_SRC_EDITOR_EDITOR_ME_PIECE_H
#define CATA_SRC_EDITOR_EDITOR_ME_PIECE_H

#include "../calendar.h"
#include "../mapgen_piece.h"
#include "../enum_conversions.h"

#include "editor_me_editable_id.h"
#include "editor_me_int_range.h"

#include <memory>
#include <string>

class JsonOut;
class JsonObject;

#define IMPLEMENT_ME_PIECE(piece_class, piece_type)                     \
    piece_class() = default;                                            \
    piece_class( const piece_class& ) = default;                        \
    piece_class( piece_class&&) = default;                              \
    ~piece_class() = default;                                           \
    PieceType get_type() const override {                               \
        return piece_type;                                              \
    }                                                                   \
    std::unique_ptr<me_piece> clone() const override {                  \
        return std::make_unique<piece_class>( *this );                  \
    };                                                                  \
    void serialize( JsonOut &jsout ) const override;                    \
    void deserialize( JsonObject &jsin ) override;                      \
    void export_func( JsonOut& jo ) const override;                     \
    void show_ui( me_state& state ) override;

namespace editor
{
struct me_state;

using PieceType = JmPieceType;

struct me_piece {
    me_piece() = default;
    virtual ~me_piece() = default;

    virtual PieceType get_type() const = 0;

    virtual std::unique_ptr<me_piece> clone() const = 0;

    virtual void serialize( JsonOut &jsout ) const = 0;
    virtual void deserialize( JsonObject &jsin ) = 0;
    virtual void export_func( JsonOut &jo ) const = 0;

    virtual void show_ui( me_state &state ) = 0;
};

struct me_piece_field : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_field, PieceType::Field );

    field_eid ftype;
    int intensity = 1;
    time_duration age = 0_seconds;
};

struct me_piece_npc : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_npc, PieceType::NPC );

    npc_template_eid npc_class;
    bool target = false;
    std::vector<trait_eid> traits;
};

struct me_piece_faction : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_faction, PieceType::Faction );

    std::string id;
};

struct me_piece_sign : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_sign, PieceType::Sign );

    bool use_snippet = false;
    snippet_category_eid snippet;
    std::string text;
};

struct me_piece_graffiti : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_graffiti, PieceType::Graffiti );

    bool use_snippet = false;
    snippet_category_eid snippet;
    std::string text;
};

struct me_piece_vending_machine : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_vending_machine, PieceType::VendingMachine );

    bool reinforced = false;
    bool use_default_group = true;
    std::string item_group;
};

struct me_piece_toilet : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_toilet, PieceType::Toilet );

    bool use_default_amount = true;
    me_int_range amount;
};

struct me_piece_gaspump : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_gaspump, PieceType::GasPump );

    // TODO
};

struct me_piece_liquid : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_liquid, PieceType::Liquid );

    // TODO
};

struct me_piece_igroup : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_igroup, PieceType::Igroup );

    // TODO
};

struct me_piece_loot : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_loot, PieceType::Loot );

    // TODO
};

struct me_piece_mgroup : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_mgroup, PieceType::Mgroup );

    // TODO
};

struct me_piece_monster : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_monster, PieceType::Monster );

    // TODO
};

struct me_piece_vehicle : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_vehicle, PieceType::Vehicle );

    // TODO
};

struct me_piece_item : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_item, PieceType::Item );

    // TODO
};

struct me_piece_trap : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_trap, PieceType::Trap );

    // TODO
};

struct me_piece_furniture : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_furniture, PieceType::Furniture );

    // TODO
};

struct me_piece_terrain : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_terrain, PieceType::Terrain );

    // TODO
};

struct me_piece_ter_furn_transform : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_ter_furn_transform, PieceType::TerFurnTransform );

    // TODO
};

struct me_piece_make_rubble : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_make_rubble, PieceType::MakeRubble );

    // TODO
};

struct me_piece_computer : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_computer, PieceType::Computer );

    // TODO
};

struct me_piece_sealed_item : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_sealed_item, PieceType::SealedItem );

    // TODO
};

struct me_piece_translate : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_translate, PieceType::Translate );

    // TODO
};

struct me_piece_zone : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_zone, PieceType::Zone );

    // TODO
};

struct me_piece_nested : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_nested, PieceType::Nested );

    // TODO
};

struct me_piece_alt_trap : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_alt_trap, PieceType::AltTrap );

    // TODO
};

struct me_piece_alt_furniture : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_alt_furniture, PieceType::AltFurniture );

    // TODO
};

struct me_piece_alt_terrain : public me_piece {
    IMPLEMENT_ME_PIECE( me_piece_alt_terrain, PieceType::AltTerrain );

    // TODO
};

const std::vector<std::unique_ptr<me_piece>> &get_piece_templates();
std::unique_ptr<me_piece> make_new_piece( PieceType pt );

} // namespace editor

template<>
struct enum_traits<editor::PieceType> {
    static constexpr editor::PieceType last = editor::PieceType::NumJmTypes;
};

#endif // CATA_SRC_EDITOR_EDITOR_ME_PIECE_H
