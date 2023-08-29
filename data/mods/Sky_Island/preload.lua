gdebug.log_info("SkyIsland: preload.")

local mod = game.mod_runtime[ game.current_mod ]

game.hooks.on_overmapgen_postprocess[ #game.hooks.on_overmapgen_postprocess + 1 ] = function(...)
    return mod.place_sky_island(...)
end

game.hooks.on_game_start[ #game.hooks.on_game_start + 1 ] = function(...)
    return mod.init_new_game(...)
end

game.iuse_functions[ "ADJUST_SI_DIFFICULTY" ] = function(...)
    return mod.adjust_difficulty(...)
end
