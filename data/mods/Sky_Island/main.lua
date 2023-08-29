gdebug.log_info("SkyIsland: main.")

local mod = game.mod_runtime[ game.current_mod ]
local storage = game.mod_storage[ game.current_mod ]

mod.island_overspecial_id = OvermapSpecialId.new("HQislandspecial")
assert( mod.island_overspecial_id:is_valid() )

mod.warper_scenario_id = ScenarioId.new("scenario_warper")
assert(mod.warper_scenario_id:is_valid())

-- Somewhat counter-intuitive, but the island is designed relative to ground level
mod.island_spawn_zlev = 0

mod.diff_casual = 1
mod.diff_normal = 2
mod.diff_hard = 3
mod.diff_impossible = 4

--[[
    TODO: Since we're using locale.gettext here, and call it on game start,
    if player changes the language mid-game the strings won't use the new language until restart.
    If we had translation class we'd be using that here.
]]
mod.difficulty_data = {
    [mod.diff_casual] = {
        name = locale.gettext("Casual"),
        descr = locale.gettext("Time limits are more relaxed.\nWarp pulses occur every 90 minutes.  This means warp sickness sets in after 12 hours of being earthside, and disintegration at 18 hours.\nYou will have more time for activities and reaching the exit should generally not be a problem."),
        interval = TimeDuration.from_minutes( 90 )
    },
    [mod.diff_normal] = {
        name = locale.gettext("Normal"),
        descr = locale.gettext("This is the intended way to play.\nWarp pulses occur every 45 minutes.  This means on every expedition, you will have 6 hours to explore, fight, loot, and find the exit before warp sickness sets in, and 8 hours total before disintegration begins.\nGetting to the exit is not a cakewalk, but you should have time to explore while earthside."),
        interval = TimeDuration.from_minutes( 45 )
    },
    [mod.diff_hard] = {
        name = locale.gettext("Hard"),
        descr = locale.gettext("Time limits are strict.\nWarp pulses occur every 30 minutes.  This means warp sickness sets in after only 4 hours of being earthside, and disintegration at 6 hours.\nYou will have much less time to spare and must make getting to the exit your immediate priority."),
        interval = TimeDuration.from_minutes( 30 )
    },
    [mod.diff_impossible] = {
        name = locale.gettext("Impossible"),
        descr = locale.gettext("Time limits are extremely tight.\nWarp pulses occur every 15 minutes.  This means warp sickness sets in after only 2 hours of being earthside, and disintegration at 3 hours.\nReaching the exit alive will take all the time you can spare, and warp sickness will be common.  You will not be given mercy!"),
        interval = TimeDuration.from_minutes( 15 )
    },
}

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

mod.place_sky_island = function( overmap )
    --[[
        When looking for starting location, the game scans 3 overmaps in each 
        direction around the central overmap (0,0).
    
        So, if we want to start on our island we have to place it
        in any overmap within that area (except the central one).

        Simple solution is best solution, so let's just always
        place our island in overmap (1,1). 
    ]]
    local p_desired_om = Point.new( 1, 1 )
    local p_om = overmap:get_abs_pos()
    if p_om == p_desired_om then
        local oms = mod.island_overspecial_id:obj()
        local num_attempts = 100
        local forced, p = mod.place_oms_in_overmap( overmap, oms, num_attempts, mod.island_spawn_zlev )

        local msg = "Placed Sky Island at "..tostring(p)
        if forced then
            msg = msg.." (forced)"
        end
        gdebug.log_info(msg)
    end
end

-- Show popup with text
mod.show_popup = function(text)
    local popup = QueryPopup.new()
    popup:message(text)
    popup:allow_any_key(true)
    popup:query()
end

-- Ask player to choose the difficulty
mod.query_difficulty = function()
    local list = UiList.new()
    list:title(locale.gettext("Choose difficulty"))
    list:allow_cancel( false )
    for diff, data in ipairs( mod.difficulty_data ) do
        list:add( diff, data.name, data.descr )
    end
    return list:query()
end

mod.set_difficulty = function(diff)
    -- Save the difficulty to storage, so it persists across save/load
    storage.difficulty = diff

    local text = locale.gettext("Difficulty set to: %s.  You can use the Difficulty Adjuster if you wish to change your setting at any time.")
    local msg = string.format(text, mod.difficulty_data[diff].name )
    gapi.add_msg(msg)
end

mod.get_difficulty = function()
    return storage.difficulty
end

mod.adjust_difficulty = function( who, item, pos )
    mod.set_difficulty( mod.query_difficulty() )
    return 0
end

mod.init_new_game = function()
    if gapi.get_scenario():get_id() ~= mod.warper_scenario_id then
        -- Check that the player didn't mess up and choose the wrong scenario.
        -- It may happen, for example, if they added ALL THE MODS AT ONCE and some conflicts cropped up.
        mod.show_popup(locale.gettext(
            [[Error: Sky Islands mod only works with the "Warper" scenario.  Please create a new world and create a new character there with the "Warper" scenario.]]
        ))
        return
    end

    mod.show_popup(locale.gettext(
        "Welcome to Sky Islands.  Here, you will have to make expeditions from your floating sanctuary base to the world below.  Fight your way to the exit within the time limit and bring back whatever you can carry, but if you die, you will be returned to the island injured, and lose all equipment you were carrying."
    ))
    mod.show_popup(locale.gettext(
        "While on expedition, you will be timed by 'warp pulses', which hit you at regular intervals.  After 8 pulses, you will suffer warp sickness, which results in reduced stats.  Every pulse after that will lower your stats even further.  After the 12th pulse, you will also begin disintegrating, taking damage until you die.\n\nGet home safely to reset the timer!"
    ))
    mod.show_popup(locale.gettext(
        "Now, let's select your difficulty mode!\n\nThis only affects how long the expedition timer is.  It won't change combat difficulty or any other settings."
    ))
    mod.set_difficulty( mod.query_difficulty() )
end

mod.activate_warp_statue = function(who, pos, is_furn)
    -- TODO: implement
    gapi.add_msg("You examine the WARP statue.")
end

mod.activate_return_statue = function(who, pos, is_furn)
    -- TODO: implement
    gapi.add_msg("You examine the RETURN statue.")
end
