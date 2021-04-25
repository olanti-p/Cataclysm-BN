#include "cata_libintl.h"

#include "fstream_utils.h"
#include "string_utils.h"
#include "string_formatter.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <limits>
#include <locale>
#include <sstream>

// TODO:
// 1. tests for mo language mixing
// 2. add note on operator precedence
// 3. write docs & guides

// ==============================================================================================
// Plural forms
// ==============================================================================================

namespace cata_internal
{
unsigned long PlfNode::eval( unsigned long n ) const
{
    switch( op ) {
        case PlfOp::Mod:
            return a->eval( n ) % b->eval( n );
        case PlfOp::Eq:
            return a->eval( n ) == b->eval( n );
        case PlfOp::NotEq:
            return a->eval( n ) != b->eval( n );
        case PlfOp::GreaterEq:
            return a->eval( n ) >= b->eval( n );
        case PlfOp::Greater:
            return a->eval( n ) > b->eval( n );
        case PlfOp::LessEq:
            return a->eval( n ) <= b->eval( n );
        case PlfOp::Less:
            return a->eval( n ) < b->eval( n );
        case PlfOp::And:
            return a->eval( n ) && b->eval( n );
        case PlfOp::Or:
            return a->eval( n ) || b->eval( n );
        case PlfOp::TerCond:
            return a->eval( n ) ? b->eval( n ) : c->eval( n );
        case PlfOp::Literal:
            return literal_val;
        case PlfOp::Variable:
            return n;
        default:
            // unreachable
            assert( false );
    }
    return 0;
}

struct PlfToken {
    PlfOp kind;
    unsigned long num;
    size_t start;
    size_t len;
};

static const std::array<std::pair<std::string, PlfOp>, 14> simple_tokens = {{
        { "?", PlfOp::TerCond },
        { ":", PlfOp::TerDelim },
        { "(", PlfOp::BrOpen },
        { ")", PlfOp::BrClose },
        { "n", PlfOp::Variable },
        { "%", PlfOp::Mod },
        { "==", PlfOp::Eq },
        { "!=", PlfOp::NotEq },
        { ">=", PlfOp::GreaterEq },
        { ">", PlfOp::Greater },
        { "<=", PlfOp::LessEq },
        { "<", PlfOp::Less },
        { "&&", PlfOp::And },
        { "||", PlfOp::Or },
    }
};

struct PlfTStream {
    const std::string *s;
    size_t pos;
    size_t end;

    PlfTStream( const std::string *raw ) {
        s = raw;
        pos = 0;
        if( s ) {
            end = s->size();
        }
    }

    PlfTStream cloned() const {
        return *this;
    }

    PlfToken peek_internal() const {
        std::string tok;
        size_t pos = this->pos;
        while( pos < end ) {
            char c = ( *s )[pos];
            if( c == ' ' || c == '\n' || c == '\r' || c == '\t' ) {
                // skip whitespace
                pos++;
                continue;
            } else if( c >= '0' && c <= '9' ) {
                // number
                size_t start_pos = pos;
                while( pos < end && c >= '0' && c <= '9' ) {
                    tok.push_back( c );
                    pos++;
                    c = ( *s )[pos];
                }
                try {
                    unsigned long ul = std::stoul( tok );
                    if( ul > std::numeric_limits<uint32_t>::max() ) {
                        throw std::out_of_range( "stoul" );
                    }
                    return PlfToken{ PlfOp::Literal, ul, start_pos, pos - start_pos };
                } catch( std::logic_error err ) {
                    std::string e = string_format( "invalid number '%s' at pos %d", tok, start_pos );
                    throw std::runtime_error( e );
                }
            }
            std::string ss = s->substr( pos, end - pos );
            for( const auto &t : simple_tokens ) {
                if( string_starts_with( ss, t.first ) ) {
                    return PlfToken{ t.second, 0, pos, t.first.size() };
                }
            }
            pos++;
            std::string e = string_format( "unexpected character '%c' at pos %d", c, pos - 1 );
            throw std::runtime_error( e );
        }
        return PlfToken{ PlfOp::NumOps, 0, pos, 0 };
    }

    PlfToken get() {
        PlfToken t = peek_internal();
        pos = t.start + t.len;
        return t;
    }

    PlfToken peek() const {
        return peek_internal();
    }

    PlfTStream &skip() {
        get();
        return *this;
    }

    bool has_tokens() const {
        return peek().kind != PlfOp::NumOps;
    }
};

struct ParseRet {
    PlfNodePtr expr;
    PlfTStream ts;

    ParseRet( PlfNodePtr e, const PlfTStream &ts ) : expr( std::move( e ) ), ts( ts ) {}
};

using ParserPtr = ParseRet( * )( const PlfTStream &ts );

ParseRet plf_get_expr( const PlfTStream &ts );
ParseRet plf_get_or( const PlfTStream &ts );
ParseRet plf_get_and( const PlfTStream &ts );
ParseRet plf_get_eq( const PlfTStream &ts );
ParseRet plf_get_cmp( const PlfTStream &ts );
ParseRet plf_get_mod( const PlfTStream &ts );
ParseRet plf_get_value( const PlfTStream &ts );

static bool plf_try_binary_op( ParseRet &left, PlfOp op, ParserPtr parser )
{
    const PlfTStream &ts = left.ts;
    if( ts.peek().kind != op ) {
        // not found
        return false;
    }
    ParseRet e = parser( ts.cloned().skip() );
    left = ParseRet( std::make_unique<PlfNode>( PlfNode{
        0, std::move( left.expr ), std::move( e.expr ), nullptr, op,
    } ),
    e.ts );
    return true;
}

ParseRet plf_get_expr( const PlfTStream &ts )
{
    ParseRet e1 = plf_get_or( ts );
    if( e1.ts.peek().kind != PlfOp::TerCond ) {
        return e1;
    }
    ParseRet e2 = plf_get_expr( e1.ts.cloned().skip() );
    if( e2.ts.peek().kind != PlfOp::TerDelim ) {
        PlfToken tok = e2.ts.peek();
        std::string e = string_format( "expected ternary delimiter at pos %d", tok.start );
        throw std::runtime_error( e );
    }
    ParseRet e3 = plf_get_expr( e2.ts.cloned().skip() );
    return ParseRet( std::make_unique<PlfNode>( PlfNode{
        0, std::move( e1.expr ), std::move( e2.expr ), std::move( e3.expr ), PlfOp::TerCond,
    } ),
    e3.ts );
}

ParseRet plf_get_or( const PlfTStream &ts )
{
    ParseRet ret = plf_get_and( ts );
    plf_try_binary_op( ret, PlfOp::Or, plf_get_or );
    return ret;
}

ParseRet plf_get_and( const PlfTStream &ts )
{
    ParseRet ret = plf_get_eq( ts );
    plf_try_binary_op( ret, PlfOp::And, plf_get_and );
    return ret;
}

ParseRet plf_get_eq( const PlfTStream &ts )
{
    ParseRet ret = plf_get_cmp( ts );
    if( plf_try_binary_op( ret, PlfOp::Eq, plf_get_eq ) ) {
        return ret;
    }
    plf_try_binary_op( ret, PlfOp::NotEq, plf_get_eq );
    return ret;
}

ParseRet plf_get_cmp( const PlfTStream &ts )
{
    ParseRet ret = plf_get_mod( ts );
    if( plf_try_binary_op( ret, PlfOp::GreaterEq, plf_get_mod ) ) {
        return ret;
    }
    if( plf_try_binary_op( ret, PlfOp::Greater, plf_get_mod ) ) {
        return ret;
    }
    if( plf_try_binary_op( ret, PlfOp::LessEq, plf_get_mod ) ) {
        return ret;
    }
    plf_try_binary_op( ret, PlfOp::Less, plf_get_mod );
    return ret;
}

ParseRet plf_get_mod( const PlfTStream &ts )
{
    ParseRet ret = plf_get_value( ts );
    plf_try_binary_op( ret, PlfOp::Mod, plf_get_value );
    return ret;
}

ParseRet plf_get_value( const PlfTStream &ts )
{
    PlfToken next = ts.peek();
    if( next.kind == PlfOp::BrOpen ) {
        // '(' expr ')'
        ParseRet e = plf_get_expr( ts.cloned().skip() );
        if( e.ts.peek().kind != PlfOp::BrClose ) {
            PlfToken tok = e.ts.peek();
            std::string e = string_format( "expected closing bracket at pos %d", tok.start );
            throw std::runtime_error( e );
        }
        return ParseRet( std::move( e.expr ), e.ts.cloned().skip() );
    } else if( next.kind == PlfOp::Literal ) {
        // number
        return ParseRet( std::make_unique<PlfNode>( PlfNode{
            next.num, nullptr, nullptr, nullptr, PlfOp::Literal,
        } ), ts.cloned().skip() );
    } else if( next.kind == PlfOp::Variable ) {
        // the variable
        return ParseRet( std::make_unique<PlfNode>( PlfNode{
            0, nullptr, nullptr, nullptr, PlfOp::Variable,
        } ), ts.cloned().skip() );
    } else {
        std::string e = string_format( "expected expression at pos %d", next.start );
        throw std::runtime_error( e );
    }
}

PlfNodePtr parse_plural_rules( const std::string &s )
{
    PlfTStream tokstr( &s );
    ParseRet ret = plf_get_expr( tokstr );
    if( ret.ts.has_tokens() ) {
        PlfToken tok = ret.ts.peek();
        std::string e = string_format( "unexpected token at pos %d", tok.start );
        throw std::runtime_error( e );
    }
    return std::move( ret.expr );
}

std::string cata_internal::PlfNode::debug_dump() const
{
    std::ostringstream ss;
    ss.imbue( std::locale::classic() );
    std::string ops = "";
    switch( op ) {
        case PlfOp::TerCond:
            ss << "(" << a->debug_dump() << "?" << b->debug_dump() << ":" << c->debug_dump() << ")";
            break;
        case PlfOp::Literal:
            ss << literal_val;
            break;
        case PlfOp::Variable:
            ss << "n";
            break;
        default:
            ops = "x";
            for( const auto &it : simple_tokens ) {
                if( it.second == op ) {
                    ops = it.first;
                    break;
                }
            }
            break;
    }
    if( !ops.empty() ) {
        ss << "(" << a->debug_dump() << ops << b->debug_dump() << ")";
    }
    return ss.str();
}
} // namespace cata_internal

// ==============================================================================================
// Translation catalogue
// ==============================================================================================

constexpr cata_internal::u32 MO_STRING_RECORD_STEP = 8;

trans_catalogue trans_catalogue::load_from_file( const std::string &file_path )
{
    auto start_tick = std::chrono::steady_clock::now();

    std::stringstream buffer;
    cata_ifstream file = std::move( cata_ifstream().mode( cata_ios_mode::binary ).open( file_path ) );
    if( !file.is_open() ) {
        throw std::runtime_error( "failed to open file" );
    }
    buffer << file->rdbuf();
    if( file.fail() ) {
        throw std::runtime_error( "failed to read file" );
    }

    auto read_tick = std::chrono::steady_clock::now();

    trans_catalogue ret = trans_catalogue( buffer.str() );

    auto end_tick = std::chrono::steady_clock::now();

    int64_t diff_read = std::chrono::duration_cast<std::chrono::milliseconds>(
                            read_tick - start_tick ).count();
    int64_t diff_parse = std::chrono::duration_cast<std::chrono::milliseconds>(
                             end_tick - read_tick ).count();
    std::cerr << string_format( "[libintl] Took %d/%d ms to load %s", diff_read, diff_parse,
                                file_path ) << std::endl;
    return ret;
}

cata_internal::u8 trans_catalogue::get_u8( u32 addr ) const
{
    if( addr + 1 > buf_size() ) {
        std::string e = string_format( "tried get_u8() at addr %#x with file size %#x", addr, buf_size() );
        throw std::runtime_error( e );
    }

    return get_u8_unsafe( addr );
}

cata_internal::u32 trans_catalogue::get_u32( u32 addr ) const
{
    if( addr + 4 > buf_size() ) {
        std::string e = string_format( "tried get_u32() at addr %#x with file size %#x", addr, buf_size() );
        throw std::runtime_error( e );
    }
    return get_u32_unsafe( addr );
}

trans_catalogue::string_info trans_catalogue::get_string_info( u32 addr ) const
{
    string_info ret;
    ret.length = get_u32( addr );
    ret.address = get_u32( addr + 4 );
    return ret;
}

trans_catalogue::string_info trans_catalogue::get_string_info_unsafe( u32 addr ) const
{
    string_info ret;
    ret.length = get_u32_unsafe( addr );
    ret.address = get_u32_unsafe( addr + 4 );
    return ret;
}

std::string trans_catalogue::get_metadata() const
{
    // We're looking for a string with empty msgid and absent msgctxt and msgid_pl.
    // Since the strings are sorted in lexicographical order, this will be the first string.
    constexpr u32 METADATA_STRING_LEN = 0;

    string_info k = get_string_info_unsafe( offs_orig_table );

    if( k.length != METADATA_STRING_LEN ) {
        std::string e = string_format(
                            "invalid metadata entry (expected length %#x, got %#x)",
                            METADATA_STRING_LEN, k.length
                        );
        throw std::runtime_error( e );
    }

    string_info v = get_string_info_unsafe( offs_trans_table );
    return std::string( addr_to_cstr( v.address ) );
}

void trans_catalogue::process_file_header()
{
    constexpr u32 MO_MAGIC_NUMBER_LE = 0x950412de;
    constexpr u32 MO_MAGIC_NUMBER_BE = 0xde120495;
    constexpr u32 MO_MAGIC_NUMBER_ADDR = 0;
    constexpr u32 MO_SUPPORTED_REVISION = 0;
    constexpr u32 MO_REVISION_ADDR = 4;
    constexpr u32 MO_NUM_STRINGS_ADDR = 8;
    constexpr u32 MO_ORIG_TABLE_OFFSET_ADDR = 12;
    constexpr u32 MO_TRANS_TABLE_OFFSET_ADDR = 16;

    u32 magic = this->buf.size() > 4 ? get_u32( MO_MAGIC_NUMBER_ADDR ) : 0;
    if( magic != MO_MAGIC_NUMBER_LE && magic != MO_MAGIC_NUMBER_BE ) {
        throw std::runtime_error( "not a MO file" );
    }
    this->is_little_endian = magic == MO_MAGIC_NUMBER_LE;
    if( get_u32( MO_REVISION_ADDR ) != MO_SUPPORTED_REVISION ) {
        std::string e = string_format(
                            "expected revision %d, got %d",
                            MO_SUPPORTED_REVISION,
                            get_u32( MO_REVISION_ADDR )
                        );
        throw std::runtime_error( e );
    }

    number_of_strings = get_u32( MO_NUM_STRINGS_ADDR );
    offs_orig_table = get_u32( MO_ORIG_TABLE_OFFSET_ADDR );
    offs_trans_table = get_u32( MO_TRANS_TABLE_OFFSET_ADDR );
}

void trans_catalogue::check_string_terminators()
{
    const auto check_string = [this]( u32 addr ) {
        // Check that string with its null terminator (not included in string length)
        // does not extend beyond file boundaries.
        string_info s = get_string_info( addr );
        if( s.address + s.length + 1 > buf_size() ) {
            std::string e = string_format(
                                "string_info at %#x: extends beyond EOF (len:%#x addr:%#x file size:%#x)",
                                addr, s.length, s.address, buf_size()
                            );
            throw std::runtime_error( e );
        }
        // Also check for existence of the null byte.
        u8 terminator = get_u8( s.address + s.length );
        if( terminator != 0 ) {
            std::string e = string_format(
                                "string_info at %#x: missing null terminator",
                                addr
                            );
            throw std::runtime_error( e );
        }
    };
    for( u32 i = 0; i < number_of_strings; i++ ) {
        check_string( offs_orig_table + i * MO_STRING_RECORD_STEP );
        check_string( offs_trans_table + i * MO_STRING_RECORD_STEP );
    }
}

void trans_catalogue::check_string_plurals()
{
    // Skip 0th metadata entry
    for( u32 i = 1; i < number_of_strings; i++ ) {
        string_info info = get_string_info_unsafe( offs_orig_table + i * MO_STRING_RECORD_STEP );

        // Check for null byte - msgid/msgid_plural separator
        bool has_plurals = false;
        for( u32 j = info.address; j < info.address + info.length; j++ ) {
            if( get_u8_unsafe( j ) == 0 ) {
                has_plurals = true;
                break;
            }
        }

        if( !has_plurals ) {
            continue;
        }

        // Count null bytes - each plural form is a null-terminated string (including last one)
        u32 addr_tr = offs_trans_table + i * MO_STRING_RECORD_STEP;
        string_info info_tr = get_string_info_unsafe( addr_tr );
        unsigned long plural_forms = 0;
        for( u32 j = info_tr.address; j <= info_tr.address + info_tr.length; j++ ) {
            if( get_u8_unsafe( j ) == 0 ) {
                plural_forms += 1;
            }
        }

        // Number of plural forms should match the number specified in metadata
        if( plural_forms != this->num_plural_forms ) {
            std::string e = string_format(
                                "string_info at %#x: expected %d plural forms, got %d",
                                addr_tr, this->num_plural_forms, plural_forms
                            );
            throw std::runtime_error( e );
        }
    }
}

void trans_catalogue::check_encoding( const meta_headers &headers )
{
    {
        bool found = false;
        for( const std::string &entry : headers ) {
            if( !string_starts_with( entry, "Content-Type:" ) ) {
                continue;
            }
            found = true;
            if( entry != "Content-Type: text/plain; charset=UTF-8" ) {
                throw std::runtime_error( "unexpected value in Content-Type header (wrong charset?)" );
            }
            break;
        }
        if( !found ) {
            throw std::runtime_error( "failed to find Content-Type header" );
        }
    }
    {
        bool found = false;
        for( const std::string &entry : headers ) {
            if( !string_starts_with( entry, "Content-Transfer-Encoding:" ) ) {
                continue;
            }
            found = true;
            if( entry != "Content-Transfer-Encoding: 8bit" ) {
                throw std::runtime_error( "unexpected value in Content-Transfer-Encoding header" );
            }
            break;
        }
        if( !found ) {
            throw std::runtime_error( "failed to find Content-Transfer-Encoding header" );
        }
    }
}

trans_catalogue::plf_header_data trans_catalogue::parse_plf_header( const meta_headers &headers )
{
    constexpr unsigned long MAX_PLURAL_FORMS = 8;

    plf_header_data ret;

    // Parse Plural-Forms header.
    std::string plf_raw;
    {
        bool found = false;
        for( const std::string &entry : headers ) {
            if( !string_starts_with( entry, "Plural-Forms:" ) ) {
                continue;
            }
            found = true;
            plf_raw = entry.substr( 13 ); // length of "Plural-Forms:" string
            break;
        }
        if( !found ) {
            // Default to English rules
            ret.num = 2;
            ret.rules = cata_internal::parse_plural_rules( "n!=1" );
            return ret;
        }
    }
    std::vector<std::string> parts = string_split( plf_raw, ';' );
    if( parts.size() != 3 ) {
        throw std::runtime_error( "expected Plural-Forms header to have 2 ';' characters" );
    }

    // Parse & validate nplurals
    {
        std::string plf_n_raw = parts[0];
        if( !string_starts_with( plf_n_raw, " nplurals=" ) ) {
            throw std::runtime_error( "failed to parse Plural-Forms header" );
        }
        plf_n_raw = plf_n_raw.substr( 10 ); // 10 is length of " nplurals=" string
        try {
            ret.num = std::stoul( plf_n_raw );
        } catch( std::runtime_error err ) {
            std::string e = string_format( "failed to parse Plural-Forms nplurals number '%s': %s", plf_n_raw,
                                           err.what() );
            throw std::runtime_error( e );
        }
        if( ret.num == 0 || ret.num > MAX_PLURAL_FORMS ) {
            std::string e = string_format( "expected at most 1-%d plural forms, got %d", MAX_PLURAL_FORMS,
                                           ret.num );
            throw std::runtime_error( e );
        }
    }

    // Parse & validate plural formula
    {
        std::string plf_rules_raw = parts[1];
        if( !string_starts_with( plf_rules_raw, " plural=" ) ) {
            throw std::runtime_error( "failed to parse Plural-Forms header" );
        }
        plf_rules_raw = plf_rules_raw.substr( 8 ); // 8 is length of " plural=" string
        try {
            ret.rules = cata_internal::parse_plural_rules( plf_rules_raw );
        } catch( std::runtime_error err ) {
            std::string e = string_format( "failed to parse plural forms formula: %s", err.what() );
            throw std::runtime_error( e );
        }
    }

    return ret;
}

trans_catalogue::trans_catalogue( std::string buffer )
{
    size_t n = buffer.size();
    this->buf.reserve( n );
    this->buf.resize( n );
    memcpy( &this->buf[0], &buffer[0], n );

    set_buffer( std::move( buffer ) );
    process_file_header();
    check_string_terminators();

    std::string meta_raw = get_metadata();
    meta_headers headers = string_split( meta_raw, '\n' );

    check_encoding( headers );
    plf_header_data plf = parse_plf_header( headers );
    this->num_plural_forms = plf.num;
    this->plf_rules = std::move( plf.rules );

    check_string_plurals();
}

const char *trans_catalogue::get_nth_orig_string( u32 n ) const
{
    u32 record_addr = offs_orig_table + n * MO_STRING_RECORD_STEP;
    string_info r = get_string_info_unsafe( record_addr );

    return addr_to_cstr( r.address );
}

const char *trans_catalogue::get_nth_translation( u32 n ) const
{
    u32 record_addr = offs_trans_table + n * MO_STRING_RECORD_STEP;
    string_info r = get_string_info_unsafe( record_addr );

    return addr_to_cstr( r.address );
}

const char *trans_catalogue::get_nth_pl_translation( u32 n, unsigned long num ) const
{
    constexpr u8 PLF_SEPARATOR = 0;
    u32 record_addr = offs_trans_table + n * MO_STRING_RECORD_STEP;
    string_info r = get_string_info_unsafe( record_addr );

    unsigned long plf = plf_rules->eval( num );

    if( plf == 0 || plf >= num_plural_forms ) {
        return addr_to_cstr( r.address );
    }
    unsigned long curr_plf = 0;
    for( u32 addr = r.address; addr <= r.address + r.length; addr++ ) {
        if( get_u8_unsafe( addr ) == PLF_SEPARATOR ) {
            curr_plf += 1;
            if( plf == curr_plf ) {
                return addr_to_cstr( addr + 1 );
            }
        }
    }
    return nullptr;
}

// ==============================================================================================
// Translation library
// ==============================================================================================

std::vector<trans_library::string_descriptor>::const_iterator trans_library::find_in_table(
    const char *id ) const
{
    auto it = std::lower_bound( string_vec.begin(), string_vec.end(),
    id, [this]( const string_descriptor & a_descr, const char *b ) -> bool {
        const char *a = this->catalogues[a_descr.catalogue].get_nth_orig_string( a_descr.entry );
        return strcmp( a, b ) < 0;
    } );

    if( it != string_vec.end() ) {
        const char *found = catalogues[it->catalogue].get_nth_orig_string( it->entry );
        if( strcmp( id, found ) == 0 ) {
            return it;
        }
    }

    return string_vec.end();
}

void trans_library::build_string_table()
{
    assert( string_vec.empty() );

    for( u32 i_cat = 0; i_cat < catalogues.size(); i_cat++ ) {
        const trans_catalogue &cat = catalogues[i_cat];
        u32 num = cat.get_num_strings();
        // 0th entry is the metadata, we skip it
        string_vec.reserve( num - 1 );
        for( u32 i = 1; i < num; i++ ) {
            const char *i_cstr = cat.get_nth_orig_string( i );

            auto it = std::lower_bound( string_vec.begin(), string_vec.end(),
            i_cstr, [this]( const string_descriptor & a_descr, const char *b ) -> bool {
                const char *a = this->catalogues[a_descr.catalogue].get_nth_orig_string( a_descr.entry );
                return strcmp( a, b ) < 0;
            } );

            string_descriptor desc = { i_cat, i };
            if( it == string_vec.end() ) {
                // Not found, or all elements are greater
                string_vec.push_back( desc );
            } else if( strcmp( catalogues[it->catalogue].get_nth_orig_string( it->entry ), i_cstr ) == 0 ) {
                // Don't overwrite existing strings
                continue;
            } else {
                string_vec.insert( it, desc );
            }
        }
    }
}

trans_library trans_library::create( std::vector<trans_catalogue> catalogues )
{
    trans_library lib;
    lib.catalogues = std::move( catalogues );

    auto start_tick = std::chrono::steady_clock::now();
    lib.build_string_table();
    auto end_tick = std::chrono::steady_clock::now();
    int64_t diff = std::chrono::duration_cast<std::chrono::milliseconds>(
                       end_tick - start_tick ).count();

    u32 num_total = 0;
    for( const trans_catalogue &cat : catalogues ) {
        num_total += cat.get_num_strings();
    }
    std::cerr << string_format( "[libintl] Took %d ms to sort %d strings", diff,
                                num_total ) << std::endl;

    return lib;
}

const char *trans_library::lookup_string_in_table( const char *id ) const
{
    auto it = find_in_table( id );
    if( it == string_vec.end() ) {
        return nullptr;
    }
    return catalogues[it->catalogue].get_nth_translation( it->entry );
}

const char *trans_library::lookup_pl_string_in_table( const char *id, unsigned long n ) const
{
    auto it = find_in_table( id );
    if( it == string_vec.end() ) {
        return nullptr;
    }
    return catalogues[it->catalogue].get_nth_pl_translation( it->entry, n );
}

const char *trans_library::get( const char *msgid ) const
{
    const char *ret = lookup_string_in_table( msgid );
    if( ret ) {
        return ret;
    } else {
        return msgid;
    }
}

const char *trans_library::get_pl( const char *msgid, const char *msgid_pl, unsigned long n ) const
{
    const char *ret = lookup_pl_string_in_table( msgid, n );
    if( ret ) {
        return ret;
    } else {
        return ( n == 1 ) ? msgid : msgid_pl;
    }
}

const char *trans_library::get_ctx( const char *ctx, const char *msgid ) const
{
    std::string buf;
    buf += ctx;
    buf += '\4';
    buf += msgid;
    return get( buf.c_str() );
}

const char *trans_library::get_ctx_pl( const char *ctx, const char *msgid, const char *msgid_pl,
                                       unsigned long n ) const
{
    std::string buf;
    buf += ctx;
    buf += '\4';
    buf += msgid;
    return get_pl( buf.c_str(), msgid_pl, n );
}
