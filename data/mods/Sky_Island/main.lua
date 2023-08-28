gdebug.log_info("SkyIsland: main.")

local mod = game.mod_runtime[ game.current_mod ]

mod.island_overspecial_id = OvermapSpecialId.new("HQislandspecial")
assert( mod.island_overspecial_id:is_valid() )

-- Somewhat counter-intuitive, but the island is designed relative to ground level
mod.island_spawn_zlev = 0

-- Place overmap special somewhere within the overmap at specified z level.
-- Returns 2 values: (bool, Tripoint)
-- (Whether force placement took place, and at what position oms was placed)
mod.place_oms_in_overmap = function( overmap, oms, num_attempts, zlev )
    local om_size = const.OM_OMT_SIZE
    -- Let's try N times
    for attempt = 0,num_attempts do
        local p = Tripoint.new( gapi.rng(0, om_size), gapi.rng(0, om_size), zlev )
        if overmap:can_place_special( oms, p, OvermapDir.north, false ) then
            local city = overmap:get_nearest_city( p )
            overmap:place_special( oms, p, OvermapDir.north, city, false, false )
            return false, p
        end
    end
    -- Tough luck. Let's just plop it right in the center.
    local p = Tripoint.new(om_size // 2, om_size // 2, zlev)
    local city = overmap:get_nearest_city( p )
    overmap:place_special( oms, p, OvermapDir.north, city, false, true )
    return true, p
end

mod.place_sky_island = function( overmap, p_om )
    --[[
        When looking for starting location, the game scans 3 overmaps in each 
        direction around the central overmap (0,0).
    
        So, if we want to start on our island we have to place it
        in any overmap within that area (except the central one).

        Simple solution is best solution, so let's just always
        place our island in overmap (1,1). 
    ]]
    local p_desired_om = Point.new( 1, 1 )
    if p_om == p_desired_om then
        local oms = mod.island_overspecial_id:obj()
        local num_attempts = 0
        local forced, p = mod.place_oms_in_overmap( overmap, oms, num_attempts, mod.island_spawn_zlev )

        local msg = "Placed Sky Island at "..tostring(p)
        if forced then
            msg = msg.." (forced)"
        end
        gdebug.log_info(msg)
    end
end
