#pragma once
#ifndef CATA_SRC_MAIN_MENU_H
#define CATA_SRC_MAIN_MENU_H

#include <cstddef>
#include <string>
#include <vector>

#include "cursesdef.h"
#include "input.h"
#include "point.h"
#include "worldfactory.h"
#include "enums.h"
#include "memory_fast.h"

class ui_adaptor;

class main_menu
{
    public:
        main_menu() : ctxt( "MAIN_MENU" ) { }
        // Shows the main menu and returns whether a game was started or not
        bool opening_screen();

    private:
        // ASCII art that says "Cataclysm Dark Days Ahead"
        std::vector<std::string> mmenu_title;
        std::string mmenu_motd;
        std::string mmenu_credits;
        std::vector<std::string> entries_main;
        std::vector<std::string> entries_new_game;
        std::vector<std::string> entries_world;
        std::vector<std::string> entries_settings;
        std::vector<std::string> hints_new_game;
        std::vector< std::vector<std::string> > hotkeys_main;
        std::vector< std::vector<std::string> > hotkeys_new_game;
        std::vector< std::vector<std::string> > hotkeys_world;
        std::vector< std::vector<std::string> > hotkeys_settings;
        std::string vdaytip; //tip of the day

        /**
         * Does what it sounds like, but this function also exists in order to gracefully handle
         * the case where the player goes to the 'Settings' tab and changes the language.
        */
        void init_strings();
        /** Helper function for @ref init_strings */
        std::vector<std::string> load_file( const std::string &path,
                                            const std::string &alt_text ) const;

        // Play a sound whenever the user moves left or right in the main menu or its tabs
        void on_move() const;

        // Handle left/right movement within a layer.
        // Returns true if movement happened
        bool move_left_right( const std::string &action, int &sel, int num );
        // Handle up/down movement within a layer.
        // Returns true if movement happened
        bool move_up_down( const std::string &action, int &sel, int num );

        // Flag to be set when first entering an error condition, cleared when leaving it
        // Used to prevent error sound from playing repeatedly at input polling rate
        bool errflag = false;
        // Play a sound *once* when an error occurs in the main menu or its tabs; sets errflag
        void on_error();
        // Clears errflag
        void clear_error();

        std::unique_ptr<ui_adaptor> make_ui_layer();

        bool do_main_layer();
        bool do_new_game_layer();
        bool do_new_game_from_preset_layer();
        bool do_load_world_layer();
        bool do_load_character_layer();
        void do_world_list_layer();
        bool do_world_action_layer();
        bool do_special_layer();
        void do_settings_layer();

        /*
         * Load character templates from template folder
         */
        void load_char_templates();

        // These variables are shared between @opening_screen and the tab functions.
        // TODO: But this is an ugly short-term solution.
        input_context ctxt;

        int layer = 0;
        int sel_main = 0;
        int sel_new_game = 0;
        int sel_preset = 0;
        int sel_load_world = 0;
        int sel_load_character = 0;
        int sel_settings = 0;
        int sel_world_list = 0;
        int sel_world_action = 0;
        int sel_special = 0;
        int text_scroll_pos = 0;

        std::string selected_world;

        int sel2 = 1;
        point LAST_TERM;
        catacurses::window w_open;
        point menu_offset;
        std::vector<std::string> templates;
        int extra_w = 0;
        std::vector<save_t> savegames;

        /**
         * Prints a horizontal list of options
         *
         * @param w_in Window we are printing in
         * @param vItems Main menu items
         * @param iSel Which index of vItems is selected. This menu item will be highlighted to
         * make it stand out from the other menu items.
         * @param offset Offset of menu items
         * @param spacing: How many spaces to print between each menu item
         */
        void print_menu_items( const catacurses::window &w_in,
                               const std::vector<std::string> &vItems, size_t iSel,
                               point offset, int spacing = 1 );

        /**
         * Called by @ref opening_screen, this prints all the text that you see on the main menu
         *
         * @param w_open Window to print menu in
         * @param iSel which index in vMenuItems is selected
         * @param offset Menu location in window
         */
        void print_menu( const catacurses::window &w_open, int iSel, point offset );

        void display_text( const std::string &text, const std::string &title, int &selected );

        void init_windows();

        /* holiday functions and member variables*/
        static bool is_easter( int day, int month, int year );
        holiday get_holiday_from_time();

        holiday current_holiday = holiday::none;

        static std::string halloween_spider();
        std::string halloween_graves();
};

#endif // CATA_SRC_MAIN_MENU_H

