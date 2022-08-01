module;

export module action_id_m;

export {

/**
 * Enumerates all discrete actions that can be performed by player
 */
enum action_id : int {
    /** Invalid action used for various lookup errors */
    ACTION_NULL = 0,

    // Mouse actions
    /**@{*/
    /** Click on a point with primary mouse button (usually left button) */
    ACTION_SELECT,
    /** Click on a point with secondary mouse button (usually right button) */
    ACTION_SEC_SELECT,
    /**@}*/

    // Character movement actions
    /**@{*/
    /** Pause an on-going activity. */
    ACTION_PAUSE,
    /** Input timeout */
    ACTION_TIMEOUT,
    /** Move towards top of screen / accelerate */
    ACTION_MOVE_FORTH,
    /** Move towards top-right of screen / accelerate and steer right */
    ACTION_MOVE_FORTH_RIGHT,
    /** Move / steer right */
    ACTION_MOVE_RIGHT,
    /** Move towards bottom-right of screen / decelerate and steer right */
    ACTION_MOVE_BACK_RIGHT,
    /** Move towards bottom of screen / decelerate */
    ACTION_MOVE_BACK,
    /** Move towards bottom-left of screen / decelerate and steer left */
    ACTION_MOVE_BACK_LEFT,
    /** Move / steer left */
    ACTION_MOVE_LEFT,
    /** Move towards top-left of screen / accelerate and steer left */
    ACTION_MOVE_FORTH_LEFT,
    /** Descend a staircase */
    ACTION_MOVE_DOWN,
    /** Ascend a staircase */
    ACTION_MOVE_UP,
    /** Cycle run/walk/crouch mode */
    ACTION_CYCLE_MOVE,
    /** Reset movement mode to walk  */
    ACTION_RESET_MOVE,
    /** Toggle run on/off */
    ACTION_TOGGLE_RUN,
    /** Toggle crouch on/off */
    ACTION_TOGGLE_CROUCH,
    /** Open movement mode menu */
    ACTION_OPEN_MOVEMENT,
    /**@}*/

    // Viewport movement actions and related
    /**@{*/
    /** Toggle memorized tiles being shown */
    ACTION_TOGGLE_MAP_MEMORY,
    /** Center the viewport on character */
    ACTION_CENTER,
    /** Move viewport north */
    ACTION_SHIFT_N,
    /** Move viewport north-east */
    ACTION_SHIFT_NE,
    /** Move viewport east */
    ACTION_SHIFT_E,
    /** Move viewport south-east */
    ACTION_SHIFT_SE,
    /** Move viewport south */
    ACTION_SHIFT_S,
    /** Move viewport south-west */
    ACTION_SHIFT_SW,
    /** Move viewport west */
    ACTION_SHIFT_W,
    /** Move viewport north-west */
    ACTION_SHIFT_NW,
    /**@}*/

    // Environment Interaction Actions
    /**@{*/
    /** Open an item (e.g. a door) */
    ACTION_OPEN,
    /** Close an item (e.g. a door) */
    ACTION_CLOSE,
    /** Smash something */
    ACTION_SMASH,
    /** Examine or pick up items from adjacent square */
    ACTION_EXAMINE,
    /** Pick up items from current/adjacent squares */
    ACTION_PICKUP,
    /** Pick up items from current square. Auto pickup if only one item */
    ACTION_PICKUP_FEET,
    /** Grab or let go of an object */
    ACTION_GRAB,
    /** Haul pile of items, or let go of them */
    ACTION_HAUL,
    /** Butcher or disassemble objects in current square */
    ACTION_BUTCHER,
    /** Chat with something */
    ACTION_CHAT,
    /** Toggle look mode */
    ACTION_LOOK,
    /** Peek through something (e.g. out of a curtained window) */
    ACTION_PEEK,
    /** List items and monsters in a given square */
    ACTION_LIST_ITEMS,
    /** Open the zone manager */
    ACTION_ZONES,
    /** Sort out the loot */
    ACTION_LOOT,
    /**@}*/

    // Inventory Interaction (including quasi-inventories like bionics)
    /**@{*/
    /** Open the primary inventory screen */
    ACTION_INVENTORY,
    /** Open the advanced inventory screen */
    ACTION_ADVANCEDINV,
    /** Open the item compare screen */
    ACTION_COMPARE,
    /** Swap inventory letters */
    ACTION_ORGANIZE,
    /** Open the use menu */
    ACTION_USE,
    /** Use currently wielded item */
    ACTION_USE_WIELDED,
    /** Open the wear clothing selection menu */
    ACTION_WEAR,
    /** Open the take-off clothing selection menu */
    ACTION_TAKE_OFF,
    /** Open the default consume item menu */
    ACTION_EAT,
    /** Open the custom consume item menu */
    ACTION_OPEN_CONSUME,
    /** Open the read menu */
    ACTION_READ,
    /** Open the wield menu */
    ACTION_WIELD,
    /** Open the martial-arts style menu */
    ACTION_PICK_STYLE,
    /** Open the load item (e.g. firearms) select menu */
    ACTION_RELOAD_ITEM,
    /** Attempt to reload wielded weapon, then fall back to the load item select menu */
    ACTION_RELOAD_WEAPON,
    /** Attempt to reload wielded object*/
    ACTION_RELOAD_WIELDED,
    /** Open the unload item (e.g. firearms) select menu */
    ACTION_UNLOAD,
    /** Open the mending menu (e.g. when using a sewing kit) */
    ACTION_MEND,
    /** Open the throw menu */
    ACTION_THROW,
    /** Fire the wielded weapon, or open fire menu if none */
    ACTION_FIRE,
    /** Burst-fire the current weapon */
    ACTION_FIRE_BURST,
    /** Change fire mode of the current weapon */
    ACTION_SELECT_FIRE_MODE,
    /** Change default ammo for current weapon */
    ACTION_SELECT_DEFAULT_AMMO,
    /** Cast a spell (only if any spells are known) */
    ACTION_CAST_SPELL,
    /** Open the drop-item menu */
    ACTION_DROP,
    /** Drop items in a given direction */
    ACTION_DIR_DROP,
    /** Open the bionics menu */
    ACTION_BIONICS,
    /** Open the mutations menu */
    ACTION_MUTATIONS,
    /** Open the armor sorting menu */
    ACTION_SORT_ARMOR,
    /** Auto select and attack hostile creature within range */
    ACTION_AUTOATTACK,
    /**@}*/

    // Long-term / special actions
    /**@{*/
    /** Open wait menu */
    ACTION_WAIT,
    /** Open crafting menu */
    ACTION_CRAFT,
    /** Repeat last craft command */
    ACTION_RECRAFT,
    /** Open batch crafting menu */
    ACTION_LONGCRAFT,
    /** Open construct menu */
    ACTION_CONSTRUCT,
    /** Open disassemble menu */
    ACTION_DISASSEMBLE,
    /** Open sleep menu */
    ACTION_SLEEP,
    /** Open vehicle control menu */
    ACTION_CONTROL_VEHICLE,
    /** Turn auto travel mode on/off */
    ACTION_TOGGLE_AUTO_TRAVEL_MODE,
    /** Turn safemode on/off, while leaving autosafemode intact */
    ACTION_TOGGLE_SAFEMODE,
    /** Turn automatic triggering of safemode on/off */
    ACTION_TOGGLE_AUTOSAFE,
    /** Toggle permanent attitude to stealing */
    ACTION_TOGGLE_THIEF_MODE,
    /** Ignore the enemy that triggered safemode */
    ACTION_IGNORE_ENEMY,
    /** Whitelist the enemy that triggered safemode */
    ACTION_WHITELIST_ENEMY,
    /** Save the game and quit */
    ACTION_SAVE,
    /** Quicksave the game */
    ACTION_QUICKSAVE,
    /** Quickload the game */
    ACTION_QUICKLOAD,
    /** Commit suicide */
    ACTION_SUICIDE,
    /**@}*/

    // Info Screens
    /**@{*/
    /** Display player status screen */
    ACTION_PL_INFO,
    /** Display over-map */
    ACTION_MAP,
    /** Show sky state for trying to predict weather */
    ACTION_SKY,
    /** Display missions screen */
    ACTION_MISSIONS,
    /** Display scores screen */
    ACTION_SCORES,
    /** Display factions screen */
    ACTION_FACTIONS,
    /** Display morale effects screen */
    ACTION_MORALE,
    /** Display messages screen */
    ACTION_MESSAGES,
    /** Display help screen */
    ACTION_HELP,
    /** Display main menu */
    ACTION_MAIN_MENU,
    /** Display keybindings list */
    ACTION_KEYBINDINGS,
    /** Display options window */
    ACTION_OPTIONS,
    /** Open autopickup manager */
    ACTION_AUTOPICKUP,
    /** Open autonotes manager */
    ACTION_AUTONOTES,
    /** Open safemode manager */
    ACTION_SAFEMODE,
    /** Open color manager */
    ACTION_COLOR,
    /** Open active world mods */
    ACTION_WORLD_MODS,
    /**@}*/

    // Debug Functions
    /**@{*/
    /** Toggle full-screen mode */
    ACTION_TOGGLE_FULLSCREEN,
    /** Open debug menu */
    ACTION_DEBUG,
    /** Toggle scent map */
    ACTION_DISPLAY_SCENT,
    /** Toggle scent type map */
    ACTION_DISPLAY_SCENT_TYPE,
    /** Toggle debug mode */
    ACTION_TOGGLE_DEBUG_MODE,
    /** Zoom view in */
    ACTION_ZOOM_OUT,
    /** Zoom view out */
    ACTION_ZOOM_IN,
    /** Open the action menu */
    ACTION_ACTIONMENU,
    /** Open the item uses menu */
    ACTION_ITEMACTION,
    /** Turn pixel minimap on/off */
    ACTION_TOGGLE_PIXEL_MINIMAP,
    /** Turn admin panel on/off */
    ACTION_TOGGLE_PANEL_ADM,
    /** panels management */
    ACTION_PANEL_MGMT,
    /** Reload current tileset */
    ACTION_RELOAD_TILESET,
    /** Turn auto features on/off */
    ACTION_TOGGLE_AUTO_FEATURES,
    /** Change auto pulp/butcher mode */
    ACTION_TOGGLE_AUTO_PULP_BUTCHER,
    /** Turn auto mining on/off */
    ACTION_TOGGLE_AUTO_MINING,
    /** Turn auto foraging on/off */
    ACTION_TOGGLE_AUTO_FORAGING,
    /** Turn auto pickup on/off */
    ACTION_TOGGLE_AUTO_PICKUP,
    /** Toggle temperature map */
    ACTION_DISPLAY_TEMPERATURE,
    /** Toggle vehicle autopilot data */
    ACTION_DISPLAY_VEHICLE_AI,
    /** Toggle visibility map */
    ACTION_DISPLAY_VISIBILITY,
    /** Toggle lighting conditions map */
    ACTION_DISPLAY_LIGHTING,
    /** Toggle radiation map */
    ACTION_DISPLAY_RADIATION,
    /** Toggle transparency map */
    ACTION_DISPLAY_TRANSPARENCY,
    /** Toggle submap grid overlay */
    ACTION_DISPLAY_SUBMAP_GRID,
    /** Toggle timing of the game hours */
    ACTION_TOGGLE_HOUR_TIMER,
    /** Not an action, serves as count of enumerated actions */
    NUM_ACTIONS
    /**@}*/
};

}
