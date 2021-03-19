#include "cata_utility.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <exception>
#include <iterator>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "catacharset.h"
#include "debug.h"
#include "filesystem.h"
#include "json.h"
#include "options.h"
#include "output.h"
#include "rng.h"
#include "translations.h"
#include "units.h"

static double pow10( unsigned int n )
{
    double ret = 1;
    double tmp = 10;
    while( n ) {
        if( n & 1 ) {
            ret *= tmp;
        }
        tmp *= tmp;
        n >>= 1;
    }
    return ret;
}

double round_up( double val, unsigned int dp )
{
    // Some implementations of std::pow does not return the accurate result even
    // for small powers of 10, so we use a specialized routine to calculate them.
    const double denominator = pow10( dp );
    return std::ceil( denominator * val ) / denominator;
}

int modulo( int v, int m )
{
    // C++11: negative v and positive m result in negative v%m (or 0),
    // but this is supposed to be mathematical modulo: 0 <= v%m < m,
    const int r = v % m;
    // Adding m in that (and only that) case.
    return r >= 0 ? r : r + m;
}

bool isBetween( int test, int down, int up )
{
    return test > down && test < up;
}

bool lcmatch( const std::string &str, const std::string &qry )
{
    if( std::locale().name() != "en_US.UTF-8" && std::locale().name() != "C" ) {
        auto &f = std::use_facet<std::ctype<wchar_t>>( std::locale() );
        std::wstring wneedle = utf8_to_wstr( qry );
        std::wstring whaystack = utf8_to_wstr( str );

        f.tolower( &whaystack[0], &whaystack[0] + whaystack.size() );
        f.tolower( &wneedle[0], &wneedle[0] + wneedle.size() );

        return whaystack.find( wneedle ) != std::wstring::npos;
    }
    std::string needle;
    needle.reserve( qry.size() );
    std::transform( qry.begin(), qry.end(), std::back_inserter( needle ), tolower );

    std::string haystack;
    haystack.reserve( str.size() );
    std::transform( str.begin(), str.end(), std::back_inserter( haystack ), tolower );

    return haystack.find( needle ) != std::string::npos;
}

bool lcmatch( const translation &str, const std::string &qry )
{
    return lcmatch( str.translated(), qry );
}

bool lcequal( const std::string &str1, const std::string &str2 )
{
    return to_lower_case( str1 ) == to_lower_case( str2 );
}

bool match_include_exclude( const std::string &text, std::string filter )
{
    size_t iPos;
    bool found = false;

    if( filter.empty() ) {
        return false;
    }

    do {
        iPos = filter.find( ',' );

        std::string term = iPos == std::string::npos ? filter : filter.substr( 0, iPos );
        const bool exclude = term.substr( 0, 1 ) == "-";
        if( exclude ) {
            term = term.substr( 1 );
        }

        if( ( !found || exclude ) && lcmatch( text, term ) ) {
            if( exclude ) {
                return false;
            }

            found = true;
        }

        if( iPos != std::string::npos ) {
            filter = filter.substr( iPos + 1, filter.size() );
        }
    } while( iPos != std::string::npos );

    return found;
}

// --- Library functions ---
// This stuff could be moved elsewhere, but there
// doesn't seem to be a good place to put it right now.

double logarithmic( double t )
{
    return 1 / ( 1 + std::exp( -t ) );
}

double logarithmic_range( int min, int max, int pos )
{
    const double LOGI_CUTOFF = 4;
    const double LOGI_MIN = logarithmic( -LOGI_CUTOFF );
    const double LOGI_MAX = logarithmic( +LOGI_CUTOFF );
    const double LOGI_RANGE = LOGI_MAX - LOGI_MIN;

    if( min >= max ) {
        debugmsg( "Invalid interval (%d, %d).", min, max );
        return 0.0;
    }

    // Anything beyond (min,max) gets clamped.
    if( pos <= min ) {
        return 1.0;
    } else if( pos >= max ) {
        return 0.0;
    }

    // Normalize the pos to [0,1]
    double range = max - min;
    double unit_pos = ( pos - min ) / range;

    // Scale and flip it to [+LOGI_CUTOFF,-LOGI_CUTOFF]
    double scaled_pos = LOGI_CUTOFF - 2 * LOGI_CUTOFF * unit_pos;

    // Get the raw logistic value.
    double raw_logistic = logarithmic( scaled_pos );

    // Scale the output to [0,1]
    return ( raw_logistic - LOGI_MIN ) / LOGI_RANGE;
}

int bound_mod_to_vals( int val, int mod, int max, int min )
{
    if( val + mod > max && max != 0 ) {
        mod = std::max( max - val, 0 );
    }
    if( val + mod < min && min != 0 ) {
        mod = std::min( min - val, 0 );
    }
    return mod;
}

double clamp_to_width( double value, int width, int &scale )
{
    return clamp_to_width( value, width, scale, nullptr );
}

double clamp_to_width( double value, int width, int &scale, bool *out_truncated )
{
    if( out_truncated != nullptr ) {
        *out_truncated = false;
    }
    if( value >= std::pow( 10.0, width ) ) {
        // above the maximum number we can fit in the width without decimal
        // show the biggest number we can without decimal
        // flag as truncated
        value = std::pow( 10.0, width ) - 1.0;
        scale = 0;
        if( out_truncated != nullptr ) {
            *out_truncated = true;
        }
    } else if( scale > 0 ) {
        for( int s = 1; s <= scale; s++ ) {
            // 1 decimal separator + "s"
            int scale_width = 1 + s;
            if( width > scale_width && value >= std::pow( 10.0, width - scale_width ) ) {
                // above the maximum number we can fit in the width with "s" decimals
                // show this number with one less decimal than "s"
                scale = s - 1;
                break;
            }
        }
    }
    return value;
}

float multi_lerp( const std::vector<std::pair<float, float>> &points, float x )
{
    size_t i = 0;
    while( i < points.size() && points[i].first <= x ) {
        i++;
    }

    if( i == 0 ) {
        return points.front().second;
    } else if( i >= points.size() ) {
        return points.back().second;
    }

    // How far are we along the way from last threshold to current one
    const float t = ( x - points[i - 1].first ) /
                    ( points[i].first - points[i - 1].first );

    // Linear interpolation of values at relevant thresholds
    return ( t * points[i].second ) + ( ( 1 - t ) * points[i - 1].second );
}

std::string obscure_message( const std::string &str, std::function<char()> f )
{
    //~ translators: place some random 1-width characters here in your language if possible, or leave it as is
    std::string gibberish_narrow = _( "abcdefghijklmnopqrstuvwxyz" );
    std::string gibberish_wide =
        //~ translators: place some random 2-width characters here in your language if possible, or leave it as is
        _( "に坂索トし荷測のンおク妙免イロコヤ梅棋厚れ表幌" );
    std::wstring w_gibberish_narrow = utf8_to_wstr( gibberish_narrow );
    std::wstring w_gibberish_wide = utf8_to_wstr( gibberish_wide );
    std::wstring w_str = utf8_to_wstr( str );
    // a trailing NULL terminator is necessary for utf8_width function
    char transformation[2] = { 0 };
    for( size_t i = 0; i < w_str.size(); ++i ) {
        transformation[0] = f();
        std::string this_char = wstr_to_utf8( std::wstring( 1, w_str[i] ) );
        if( transformation[0] == -1 ) {
            continue;
        } else if( transformation[0] == 0 ) {
            if( utf8_width( this_char ) == 1 ) {
                w_str[i] = random_entry( w_gibberish_narrow );
            } else {
                w_str[i] = random_entry( w_gibberish_wide );
            }
        } else {
            // Only support the case e.g. replace current character to symbols like # or ?
            if( utf8_width( transformation ) != 1 ) {
                debugmsg( "target character isn't narrow" );
            }
            // A 2-width wide character in the original string should be replace by two narrow characters
            w_str.replace( i, 1, utf8_to_wstr( std::string( utf8_width( this_char ), transformation[0] ) ) );
        }
    }
    std::string result = wstr_to_utf8( w_str );
    if( utf8_width( str ) != utf8_width( result ) ) {
        debugmsg( "utf8_width differ between original string and obscured string" );
    }
    return result;
}

bool string_starts_with( const std::string &s1, const std::string &s2 )
{
    return s1.compare( 0, s2.size(), s2 ) == 0;
}

bool string_ends_with( const std::string &s1, const std::string &s2 )
{
    return s1.size() >= s2.size() &&
           s1.compare( s1.size() - s2.size(), s2.size(), s2 ) == 0;
}

std::string join( const std::vector<std::string> &strings, const std::string &joiner )
{
    std::ostringstream buffer;

    for( auto a = strings.begin(); a != strings.end(); ++a ) {
        if( a != strings.begin() ) {
            buffer << joiner;
        }
        buffer << *a;
    }
    return buffer.str();
}
