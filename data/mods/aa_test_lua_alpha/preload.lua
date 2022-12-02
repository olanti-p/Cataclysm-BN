log_info( "Preload: ", game.current_mod )

local hw = "Hello, runtime!"

game.mod_runtime[ game.current_mod ]["test"] = hw

-- This variable uses 'local', so it gets discarded as soon as script ends.
-- (Unless some function stores a reference, then it just ends up owned by function)
local test_mod_a_private = "wtf"
-- This variable doesn't use 'local' and ends up being global, 
-- but luckily we use environments, so it doesn't end up bleeding into the next called script.
test_mod_a_private_glob = "wtf"
