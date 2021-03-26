#include "options_helpers.h"

#include "options.h"
#include "weather.h"

override_option::override_option( const std::string &option, const std::string &value ) :
    option_( option )
{
    options_manager::cOpt &opt = get_options().get_option( option_ );
    old_value_ = opt.getValue( true );
    opt.setValue( value );
}

override_option::~override_option()
{
    get_options().get_option( option_ ).setValue( old_value_ );
}

bool try_set_utf8_locale()
{
    try {
        std::locale::global( std::locale( "en_US.UTF-8" ) );
    } catch( std::runtime_error & ) {
        return false;
    }
    return true;
}

scoped_weather_override::scoped_weather_override( const weather_type_id &weather )
{
    get_weather().weather_override = weather;
}

scoped_weather_override::~scoped_weather_override()
{
    get_weather().weather_override = weather_type_id::NULL_ID();
}
