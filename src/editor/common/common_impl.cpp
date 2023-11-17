#include "uuid.h"
#include "int_range.h"

#include "json.h"

namespace editor
{


void uuid_generator::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "counter", counter );
    jsout.end_object();
}

void uuid_generator::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "counter", counter );
}

void me_int_range::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member( "min", min );
    jsout.member( "max", max );
    jsout.end_object();
}

void me_int_range::deserialize( JsonIn &jsin )
{
    JsonObject jo = jsin.get_object();

    jo.read( "min", min );
    jo.read( "max", max );
}

} // namespace editor
