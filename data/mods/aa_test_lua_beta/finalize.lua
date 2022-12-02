log_info( "Finalize: ", game.current_mod )

-- Print current mod list and mod runtime state
do
    local msg = "Current mod list:\n"
    for _, mod in ipairs( game.active_mods ) do
        msg = msg..mod.."   storage:"
        for k,v in pairs( game.mod_runtime[ mod ] ) do
            msg = msg..tostring(k)..":"..tostring(v)..";"
        end
        msg = msg.."\n"
    end
    log_info( msg )
end

-- game.active_mods["asd"] = "hoho!"

error("DONE")
