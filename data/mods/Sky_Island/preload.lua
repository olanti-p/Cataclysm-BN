gdebug.log_info("SkyIsland: preload.")

local mod = game.mod_runtime[ game.current_mod ]

game.hooks.on_overmapgen_postprocess[ #game.hooks.on_overmapgen_postprocess + 1 ] = function(...)
    return mod.place_sky_island(...)
end
