#pragma once
#ifndef CATA_SRC_CATA_LIBINTL_H
#define CATA_SRC_CATA_LIBINTL_H

#include <memory>
#include <string>
#include <vector>

namespace cata_libintl
{
using u8 = uint8_t;
using u32 = uint32_t;

/**
 * Plural forms AST node type.
 * Also used for token identification during parsing.
 */
enum class PlfOp : int {
    Mod,       // a % b
    Eq,        // a == b
    NotEq,     // a != b
    GreaterEq, // a >= b
    Greater,   // a > b
    LessEq,    // a <= b
    Less,      // a < b
    And,       // a && b
    Or,        // a || b
    TerCond,   // ?

    Literal,   // numeric literal
    Variable,  // the variable (n)

    BrOpen,    // (
    BrClose,   // )
    TerDelim,  // :

    NumOps,
};

struct PlfNode;
using PlfNodePtr = std::unique_ptr<PlfNode>;

/** Plural forms AST node */
struct PlfNode {
    unsigned long literal_val;
    PlfNodePtr a;
    PlfNodePtr b;
    PlfNodePtr c;
    PlfOp op = PlfOp::NumOps;

    unsigned long eval( unsigned long n ) const;
    std::string debug_dump() const;
};

/**
 * Parse plural rules expression and build AST.
 * @throws on failure.
 */
PlfNodePtr parse_plural_rules( const std::string &s );

/**
 * Translation catalogue. Corresponds to single MO file.
 *
 * For reference on MO files, see 'GNU gettext utilities' manual:
 * https://www.gnu.org/software/gettext/manual/html_node/MO-Files.html
 */
class trans_catalogue
{
    private:
        // Represents 1 entry in MO string table
        struct string_descr {
            u32 length;
            u32 offset;
        };
        struct catalogue_plurals_info {
            unsigned long num = 0;
            PlfNodePtr expr = nullptr;
        };
        using meta_headers = std::vector<std::string>;

        // =========== MEMBERS ===========

        bool is_little_endian = true; // File endianness
        std::string buffer; // Data buffer
        catalogue_plurals_info plurals; // Plural rules
        u32 number_of_strings = 0; // Number of strings (id-translation pairs)
        u32 offs_orig_table = 0; // Offset of table with original strings
        u32 offs_trans_table = 0; // Offset of table with translated strings

        // =========== METHODS ===========

        explicit trans_catalogue( std::string buffer );

        inline void set_buffer( std::string buffer ) {
            buffer = std::move( buffer );
        }
        inline u32 buf_size() const {
            return static_cast<u32>( buffer.size() );
        }

        u8 get_u8( u32 offs ) const;
        inline u8 get_u8_unsafe( u32 offs ) const {
            return static_cast<u8>( buffer[offs] );
        }

        u32 get_u32( u32 offs ) const;
        inline u32 get_u32_unsafe( u32 offs ) const {
            if( is_little_endian ) {
                return get_u8_unsafe( offs ) |
                       get_u8_unsafe( offs + 1 ) << 8 |
                       get_u8_unsafe( offs + 2 ) << 16 |
                       get_u8_unsafe( offs + 3 ) << 24;
            } else {
                return get_u8_unsafe( offs + 3 ) |
                       get_u8_unsafe( offs + 2 ) << 8 |
                       get_u8_unsafe( offs + 1 ) << 16 |
                       get_u8_unsafe( offs ) << 24;
            }
        }

        string_descr get_string_descr( u32 offs ) const;
        string_descr get_string_descr_unsafe( u32 offs ) const;

        inline const char *offs_to_cstr( u32 offs ) const {
            return &buffer[offs];
        }

        void process_file_header();
        void check_string_terminators();
        void check_string_plurals();
        std::string get_metadata() const;
        static void check_encoding( const meta_headers &headers );
        static catalogue_plurals_info parse_plf_header( const meta_headers &headers );

    public:
        /**
         * Load translation catalogue from given MO file.
         * @throws on failure.
         */
        static trans_catalogue load_from_file( const std::string &file_path );

        /** Number of entries in the catalogue. */
        inline u32 get_num_strings() const {
            return number_of_strings;
        }
        /** Get singular translated string of given entry. */
        const char *get_nth_translation( u32 n ) const;
        /** Get correct plural translated string of given entry for given number. */
        const char *get_nth_pl_translation( u32 n, unsigned long num ) const;
        /** Get original msgid (with msgctxt) of given entry. */
        const char *get_nth_orig_string( u32 n ) const;
};

/**
 * Translation library.
 * Represents collection of catalogues merged into a single pool ready for use.
 */
class trans_library
{
    private:
        // Describes which catalogue the string comes from
        struct library_string_descr {
            u32 catalogue;
            u32 entry;
        };

        // Full index of loaded strings
        std::vector<library_string_descr> strings;

        // Full index of loaded catalogues
        std::vector<trans_catalogue> catalogues;

        void build_string_table();
        std::vector<library_string_descr>::const_iterator find_entry( const char *id ) const;
        const char *lookup_string( const char *id ) const;
        const char *lookup_pl_string( const char *id, unsigned long n ) const;

    public:
        /** Create new library from catalogues. */
        static trans_library create( std::vector<trans_catalogue> catalogues );

        /**
         * Get translated string.
         *
         * @param msgid - message id
         * @param ctx - message context
         * @param msgid_pld - message id in plural form (n!=1)
         * @param n - number to choose plural form for
         * @returns If translation is found, returns translated string. Otherwise, returns original string.
         *
         * @{
         */
        const char *get( const char *msgid ) const;
        const char *get_pl( const char *msgid, const char *msgid_pl, unsigned long n ) const;
        const char *get_ctx( const char *ctx, const char *msgid ) const;
        const char *get_ctx_pl( const char *ctx, const char *msgid, const char *msgid_pl,
                                unsigned long n ) const;
        /** @} */
};
} // cata_libintl

#endif // CATA_SRC_CATA_LIBINTL_H
