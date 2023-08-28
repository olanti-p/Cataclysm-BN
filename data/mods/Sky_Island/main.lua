gdebug.log_info("SkyIsland: main.")

local mod = game.mod_runtime[ game.current_mod ]

mod.island_overspecial_id = OvermapSpecialId.new("HQislandspecial")
assert( mod.island_overspecial_id:is_valid() )

-- Somewhat counter-intuitive, but the island is designed relative to ground level
mod.island_spawn_zlev = 0

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
        local om_size = const.OM_OMT_SIZE
        local oms = mod.island_overspecial_id:obj()
        -- Let's try 100 times
        for attempt = 0,100 do
            local p = Tripoint.new( gapi.rng(0, om_size), gapi.rng(0, om_size), mod.island_spawn_zlev )
            if overmap:can_place_special( oms, p, OvermapDir.north, false ) then
                local city = overmap:get_nearest_city( p )
                overmap:place_special( oms, p, OvermapDir.north, city, false, false )
                gdebug.log_info("Placed Sky Island at "..tostring(p))
                return
            end
        end
        -- Tough luck. How this happened is a mystery, let's just force our way through.
        local p = Tripoint.new(om_size // 2, om_size // 2, mod.island_spawn_zlev)
        local city = overmap:get_nearest_city( p )
        overmap:place_special( oms, p, OvermapDir.north, city, false, true )
        gdebug.log_info("Placed Sky Island at "..tostring(p).." (forced)")
    end
end
