#include "main_menu.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <functional>
#include <istream>
#include <memory>
#include <ctime>
#include <vector>

#include "auto_pickup.h"
#include "avatar.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_id.h"
#include "color.h"
#include "debug.h"
#include "distraction_manager.h"
#include "enums.h"
#include "filesystem.h"
#include "fstream_utils.h"
#include "game.h"
#include "gamemode.h"
#include "get_version.h"
#include "help.h"
#include "ime.h"
#include "loading_ui.h"
#include "mapbuffer.h"
#include "mapsharing.h"
#include "newcharacter.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "pldata.h"
#include "safemode_ui.h"
#include "scenario.h"
#include "sdlsound.h"
#include "sounds.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"
#include "ui_manager.h"
#include "wcwidth.h"
#include "worldfactory.h"

constexpr int LAYER_MAIN = 0;
constexpr int LAYER_NEW_GAME = 1;
constexpr int LAYER_CHOOSE_PRESET = 2;
constexpr int LAYER_LOAD_WORLD = 3;
constexpr int LAYER_LOAD_CHARACTER = 4;
constexpr int LAYER_WORLD_LIST = 5;
constexpr int LAYER_WORLD_ACTION = 6;
constexpr int LAYER_SETTINGS = 7;
constexpr int LAYER_SPECIAL = 8;

constexpr int LAY_MAIN_MOTD = 0;
constexpr int LAY_MAIN_NEW_GAME = 1;
constexpr int LAY_MAIN_LOAD_GAME = 2;
constexpr int LAY_MAIN_WORLDS = 3;
constexpr int LAY_MAIN_SPECIAL = 4;
constexpr int LAY_MAIN_SETTINGS = 5;
constexpr int LAY_MAIN_HELP = 6;
constexpr int LAY_MAIN_CREDITS = 7;
constexpr int LAY_MAIN_QUIT = 8;
constexpr int LAY_MAIN_NUM = 9;

constexpr int LAY_NEWGAME_CUSTOM = 0;
constexpr int LAY_NEWGAME_PRESET = 1;
constexpr int LAY_NEWGAME_RANDOM = 2;
constexpr int LAY_NEWGAME_PLAY_FIXED = 3;
constexpr int LAY_NEWGAME_PLAY_RANDOM = 4;
constexpr int LAY_NEWGAME_NUM = 5;

constexpr int LAY_SETTINGS_OPTIONS = 0;
constexpr int LAY_SETTINGS_KEYBINDS = 1;
constexpr int LAY_SETTINGS_AUTOPICKUP = 2;
constexpr int LAY_SETTINGS_SAFEMODE = 3;
constexpr int LAY_SETTINGS_DISTRACTIONS = 4;
constexpr int LAY_SETTINGS_COLORS = 5;
constexpr int LAY_SETTINGS_NUM = 6;

constexpr int LAY_WORLDACT_DELETE = 0;
constexpr int LAY_WORLDACT_RESET = 1;
constexpr int LAY_WORLDACT_SHOW_MODS = 2;
constexpr int LAY_WORLDACT_EDIT_MODS = 3;
constexpr int LAY_WORLDACT_COPY_SETTINGS = 4;
constexpr int LAY_WORLDACT_MAKE_TEMPL = 5;
constexpr int LAY_WORLDACT_NUM = 6;


void main_menu::on_move() const
{
    sfx::play_variant_sound( "menu_move", "default", 100 );
}

void main_menu::on_error()
{
    if( errflag ) {
        return;
    }
    sfx::play_variant_sound( "menu_error", "default", 100 );
    errflag = true;
}

void main_menu::clear_error()
{
    errflag = false;
}

//CJK characters have a width of 2, etc
static int utf8_width_notags( const char *s )
{
    int len = strlen( s );
    const char *ptr = s;
    int w = 0;
    bool inside_tag = false;
    while( len > 0 ) {
        uint32_t ch = UTF8_getch( &ptr, &len );
        if( ch == UNKNOWN_UNICODE ) {
            continue;
        }
        if( ch == '<' ) {
            inside_tag = true;
        } else if( ch == '>' ) {
            inside_tag = false;
            continue;
        }
        if( inside_tag ) {
            continue;
        }
        w += mk_wcwidth( ch );
    }
    return w;
}

bool main_menu::move_left_right( const std::string &action, int &sel, int num )
{
    if( action == "LEFT" ) {
        if( num > 0 ) {
            if( sel > 0 ) {
                sel--;
            } else {
                sel = num - 1;
            }
            on_move();
        }
        return true;
    } else if( action == "RIGHT" ) {
        if( num > 0 ) {
            if( sel < num - 1 ) {
                sel++;
            } else {
                sel = 0;
            }
            on_move();
        }
        return true;
    }
    return false;
}

bool main_menu::move_up_down( const std::string &action, int &sel, int num )
{
    if( action == "DOWN" ) {
        if( num > 0 ) {
            if( sel > 0 ) {
                sel--;
            } else {
                sel = num - 1;
            }
            on_move();
        } else {
            sel = 0;
        }
        return true;
    } else if( action == "UP" ) {
        if( num > 0 ) {
            if( sel < num - 1 ) {
                sel++;
            } else {
                sel = 0;
            }
            on_move();
        } else {
            sel = 0;
        }
        return true;
    }
    return false;
}

void main_menu::print_menu_items( const catacurses::window &w_in,
                                  const std::vector<std::string> &vItems,
                                  size_t iSel, point offset, int spacing )
{
    std::string text;
    for( size_t i = 0; i < vItems.size(); ++i ) {
        if( i > 0 ) {
            text += std::string( spacing, ' ' );
        }

        std::string temp = shortcut_text( c_white, vItems[i] );
        if( iSel == i ) {
            text += string_format( "[%s]", colorize( remove_color_tags( temp ), h_white ) );
        } else {
            text += string_format( "[%s]", temp );
        }
    }

    int text_width = utf8_width_notags( text.c_str() );
    if( text_width > getmaxx( w_in ) ) {
        offset.y -= std::ceil( text_width / getmaxx( w_in ) );
    }

    fold_and_print( w_in, offset, getmaxx( w_in ), c_light_gray, text, ']' );
}

void main_menu::print_menu( const catacurses::window &w_open, int iSel, point offset )
{
    // Clear Lines
    werase( w_open );

    // Define window size
    int window_width = getmaxx( w_open );
    int window_height = getmaxy( w_open );

    // Draw horizontal line
    for( int i = 1; i < window_width - 1; ++i ) {
        mvwputch( w_open, point( i, window_height - 4 ), c_white, LINE_OXOX );
    }

    center_print( w_open, window_height - 2, c_red,
                  _( "Bugs?  Suggestions?  Use links in MOTD to report them." ) );

    center_print( w_open, window_height - 1, c_light_cyan, string_format( _( "Tip of the day: %s" ),
                  vdaytip ) );

    int iLine = 0;
    const int iOffsetX = ( window_width - FULL_SCREEN_WIDTH ) / 2;

    switch( current_holiday ) {
        case holiday::new_year:
            break;
        case holiday::easter:
            break;
        case holiday::halloween:
            fold_and_print_from( w_open, point_zero, 30, 0, c_white, halloween_spider() );
            fold_and_print_from( w_open, point( getmaxx( w_open ) - 25, offset.y - 8 ),
                                 25, 0, c_white, halloween_graves() );
            break;
        case holiday::thanksgiving:
            break;
        case holiday::christmas:
            break;
        case holiday::none:
        case holiday::num_holiday:
        default:
            break;
    }

    if( mmenu_title.size() > 1 ) {
        for( const std::string &line : mmenu_title ) {
            nc_color cur_color = c_white;
            nc_color base_color = c_white;
            print_colored_text( w_open, point( iOffsetX, iLine++ ), cur_color, base_color, line );
        }
    } else {
        center_print( w_open, iLine++, c_light_cyan, mmenu_title[0] );
    }

    iLine++;
    center_print( w_open, iLine, c_light_blue, string_format( _( "Version: %s" ),
                  getVersionString() ) );

    int menu_length = 0;
    for( size_t i = 0; i < entries_main.size(); ++i ) {
        menu_length += utf8_width_notags( entries_main[i].c_str() ) + 2;
        if( !hotkeys_main[i].empty() ) {
            menu_length += utf8_width( hotkeys_main[i][0] );
        }
    }
    const int free_space = std::max( 0, window_width - menu_length - offset.x );
    const int spacing = free_space / ( static_cast<int>( entries_main.size() ) + 1 );
    const int width_of_spacing = spacing * ( entries_main.size() + 1 );
    const int adj_offset = std::max( 0, ( free_space - width_of_spacing ) / 2 );
    const int final_offset = offset.x + adj_offset + spacing;

    print_menu_items( w_open, entries_main, iSel, point( final_offset, offset.y ), spacing );

    wnoutrefresh( w_open );
}

std::vector<std::string> main_menu::load_file( const std::string &path,
        const std::string &alt_text ) const
{
    std::vector<std::string> result;
    read_from_file_optional( path, [&result]( std::istream & fin ) {
        std::string line;
        while( std::getline( fin, line ) ) {
            if( !line.empty() && line[0] == '#' ) {
                continue;
            }
            result.push_back( line );
        }
    } );
    if( result.empty() ) {
        result.push_back( alt_text );
    }
    return result;
}

/* compare against table of easter dates */
bool main_menu::is_easter( int day, int month, int year )
{
    if( month == 3 ) {
        switch( year ) {
            // *INDENT-OFF*
            case 2024: return day == 31;
            case 2027: return day == 28;
            default: break;
            // *INDENT-ON*
        }
    } else if( month == 4 ) {
        switch( year ) {
            // *INDENT-OFF*
            case 2021: return day == 4;
            case 2022: return day == 17;
            case 2023: return day == 9;
            case 2025: return day == 20;
            case 2026: return day == 5;
            case 2028: return day == 16;
            case 2029: return day == 1;
            case 2030: return day == 21;
            default: break;
            // *INDENT-ON*
        }
    }
    return false;
}

holiday main_menu::get_holiday_from_time()
{
    bool success = false;

    std::tm local_time;
    std::time_t current_time = std::time( nullptr );

    /* necessary to pass LGTM, as threadsafe version of localtime differs by platform */
#if defined(_WIN32)

    errno_t err = localtime_s( &local_time, &current_time );
    if( err == 0 ) {
        success = true;
    }

#else

    success = !!localtime_r( &current_time, &local_time );

#endif

    if( success ) {

        const int month = local_time.tm_mon + 1;
        const int day = local_time.tm_mday;
        const int wday = local_time.tm_wday;
        const int year = local_time.tm_year + 1900;

        /* check date against holidays */
        if( month == 1 && day == 1 ) {
            return holiday::new_year;
        }
        // only run easter date calculation if currently March or April
        else if( ( month == 3 || month == 4 ) && is_easter( day, month, year ) ) {
            return holiday::easter;
        } else if( month == 7 && day == 4 ) {
            return holiday::independence_day;
        }
        // 13 days seems appropriate for Halloween
        else if( month == 10 && day >= 19 ) {
            return holiday::halloween;
        } else if( month == 11 && ( day >= 22 && day <= 28 ) && wday == 4 ) {
            return holiday::thanksgiving;
        }
        // For the 12 days of Christmas, my true love gave to me...
        else if( month == 12 && ( day >= 14 && day <= 25 ) ) {
            return holiday::christmas;
        }
    }
    // fall through to here if localtime fails, or none of the day tests hit
    return holiday::none;
}

void main_menu::init_windows()
{
    if( LAST_TERM == point( TERMX, TERMY ) ) {
        return;
    }

    // main window should also expand to use available display space.
    // expanding to evenly use up half of extra space, for now.
    extra_w = ( ( TERMX - FULL_SCREEN_WIDTH ) / 2 ) - 1;
    int extra_h = ( ( TERMY - FULL_SCREEN_HEIGHT ) / 2 ) - 1;
    extra_w = ( extra_w > 0 ? extra_w : 0 );
    extra_h = ( extra_h > 0 ? extra_h : 0 );
    const int total_w = FULL_SCREEN_WIDTH + extra_w;
    const int total_h = FULL_SCREEN_HEIGHT + extra_h;

    // position of window within main display
    const int x0 = ( TERMX - total_w ) / 2;
    const int y0 = ( TERMY - total_h ) / 2;

    w_open = catacurses::newwin( total_h, total_w, point( x0, y0 ) );

    menu_offset.y = total_h - 3;
    // note: if iMenuOffset is changed,
    // please update MOTD and credits to indicate how long they can be.

    LAST_TERM = point( TERMX, TERMY );
}

void main_menu::init_strings()
{
    // ASCII Art
    mmenu_title = load_file( PATH_INFO::title( current_holiday ), _( "Cataclysm: Bright Nights" ) );
    // MOTD
    auto motd = load_file( PATH_INFO::motd(), _( "No message today." ) );

    mmenu_motd.clear();
    for( const std::string &line : motd ) {
        mmenu_motd += ( line.empty() ? " " : line ) + "\n";
    }
    mmenu_motd = colorize( mmenu_motd, c_light_red );

    // Credits
    mmenu_credits.clear();
    read_from_file_optional( PATH_INFO::credits(), [&]( std::istream & stream ) {
        std::string line;
        while( std::getline( stream, line ) ) {
            if( line[0] != '#' ) {
                mmenu_credits += ( line.empty() ? " " : line ) + "\n";
            }
        }
    } );

    if( mmenu_credits.empty() ) {
        mmenu_credits = _( "No credits information found." );
    }

    // fill menu with translated menu items
    entries_main.clear();
    entries_main.push_back( pgettext( "Main Menu", "<M|m>OTD" ) );
    entries_main.push_back( pgettext( "Main Menu", "<N|n>ew Game" ) );
    entries_main.push_back( pgettext( "Main Menu", "Lo<a|A>d" ) );
    entries_main.push_back( pgettext( "Main Menu", "<W|w>orld" ) );
    entries_main.push_back( pgettext( "Main Menu", "<S|s>pecial" ) );
    entries_main.push_back( pgettext( "Main Menu", "Se<t|T>tings" ) );
    entries_main.push_back( pgettext( "Main Menu", "H<e|E|?>lp" ) );
    entries_main.push_back( pgettext( "Main Menu", "<C|c>redits" ) );
    entries_main.push_back( pgettext( "Main Menu", "<Q|q>uit" ) );

    // determine hotkeys from translated menu item text
    hotkeys_main.clear();
    for( const std::string &item : entries_main ) {
        hotkeys_main.push_back( get_hotkeys( item ) );
    }

    entries_new_game.push_back( pgettext( "Main Menu|New Game", "<C|c>ustom Character" ) );
    entries_new_game.push_back( pgettext( "Main Menu|New Game", "<P|p>reset Character" ) );
    entries_new_game.push_back( pgettext( "Main Menu|New Game", "<R|r>andom Character" ) );
    if( !MAP_SHARING::isSharing() ) { // "Play Now" function doesn't play well together with shared maps
        entries_new_game.push_back( pgettext( "Main Menu|New Game", "Play Now!  (<F|f>ixed Scenario)" ) );
        entries_new_game.push_back( pgettext( "Main Menu|New Game", "Play <N|n>ow!" ) );
    }

    hotkeys_new_game.clear();
    for( const std::string &item : entries_new_game ) {
        hotkeys_new_game.push_back( get_hotkeys( item ) );
    }

    hints_new_game.push_back(
        _( "Allows you to fully customize points pool, scenario, and character's profession, stats, traits, skills and other parameters." ) );
    hints_new_game.push_back(
        _( "Select from one of previously created character templates." ) );
    hints_new_game.push_back(
        _( "Creates random character, but lets you preview the generated character and the scenario and change character and/or scenario if needed." ) );
    hints_new_game.push_back(
        _( "Puts you right in the game, randomly choosing character's traits, profession, skills and other parameters.  Scenario is fixed to Evacuee." ) );
    hints_new_game.push_back(
        _( "Puts you right in the game, randomly choosing scenario and character's traits, profession, skills and other parameters." ) );

    entries_world.clear();
    entries_world.push_back( pgettext( "Main Menu|World", "<D|d>elete World" ) );
    entries_world.push_back( pgettext( "Main Menu|World", "<R|r>eset World" ) );
    entries_world.push_back( pgettext( "Main Menu|World", "<S|s>how World Mods" ) );
    entries_world.push_back( pgettext( "Main Menu|World", "<E|e>dit World Mods" ) );
    entries_world.push_back( pgettext( "Main Menu|World", "<C|c>opy World Settings" ) );
    entries_world.push_back( pgettext( "Main Menu|World", "Character to <T|t>emplate" ) );

    hotkeys_world.clear();
    for( const std::string &item : entries_world ) {
        hotkeys_world.push_back( get_hotkeys( item ) );
    }

    entries_settings.clear();
    entries_settings.push_back( pgettext( "Main Menu|Settings", "<O|o>ptions" ) );
    entries_settings.push_back( pgettext( "Main Menu|Settings", "K<e|E>ybindings" ) );
    entries_settings.push_back( pgettext( "Main Menu|Settings", "<A|a>utopickup" ) );
    entries_settings.push_back( pgettext( "Main Menu|Settings", "<S|s>afemode" ) );
    entries_settings.push_back( pgettext( "Main Menu|Settings", "<D|d>istractions" ) );
    entries_settings.push_back( pgettext( "Main Menu|Settings", "<C|c>olors" ) );

    hotkeys_settings.clear();
    for( const std::string &item : entries_settings ) {
        hotkeys_settings.push_back( get_hotkeys( item ) );
    }

    vdaytip = get_random_tip_of_the_day();
}

void main_menu::display_text( const std::string &text, const std::string &title, int &selected )
{
    catacurses::window w_border = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                  point( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0,
                                         TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) );

    catacurses::window w_text = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                                point( 1 + static_cast<int>( TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0 ),
                                       1 + static_cast<int>( TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0 ) ) );

    draw_border( w_border, BORDER_COLOR, title );

    int width = FULL_SCREEN_WIDTH - 2;
    int height = FULL_SCREEN_HEIGHT - 2;
    const auto vFolded = foldstring( text, width );
    int iLines = vFolded.size();

    if( selected < 0 ) {
        selected = 0;
    } else if( iLines < height ) {
        selected = 0;
    } else if( selected >= iLines - height ) {
        selected = iLines - height;
    }

    fold_and_print_from( w_text, point_zero, width, selected, c_light_gray, text );

    draw_scrollbar( w_border, selected, height, iLines, point_south, BORDER_COLOR, true );
    wnoutrefresh( w_border );
    wnoutrefresh( w_text );
}

void main_menu::load_char_templates()
{
    templates.clear();

    for( std::string path : get_files_from_path( ".template", PATH_INFO::templatedir(), false,
            true ) ) {
        path.erase( path.find( ".template" ), std::string::npos );
        path.erase( 0, path.find_last_of( "\\/" ) + 1 );
        templates.push_back( path );
    }
    std::sort( templates.begin(), templates.end(), localized_compare );
    std::reverse( templates.begin(), templates.end() );
}

bool main_menu::opening_screen()
{
    // set holiday based on local system time
    current_holiday = get_holiday_from_time();

    // Play title music, whoo!
    play_music( "title" );

    world_generator->set_active_world( nullptr );
    world_generator->init();

    get_help().load();
    init_strings();

    if( !assure_dir_exist( PATH_INFO::config_dir() ) ) {
        popup( _( "Unable to make config directory.  Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( PATH_INFO::savedir() ) ) {
        popup( _( "Unable to make save directory.  Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( PATH_INFO::templatedir() ) ) {
        popup( _( "Unable to make templates directory.  Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( PATH_INFO::user_fontdir() ) ) {
        popup( _( "Unable to make fonts directory.  Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( PATH_INFO::user_sound() ) ) {
        popup( _( "Unable to make sound directory.  Check permissions." ) );
        return false;
    }

    if( !assure_dir_exist( PATH_INFO::user_gfx() ) ) {
        popup( _( "Unable to make graphics directory.  Check permissions." ) );
        return false;
    }

    load_char_templates();

    ctxt.register_cardinal();
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "DELETE_TEMPLATE" );
    ctxt.register_action( "PAGE_UP" );
    ctxt.register_action( "PAGE_DOWN" );
    // for the menu shortcuts
    ctxt.register_action( "ANY_INPUT" );

    g->u = avatar();

    background_pane background;

    std::unique_ptr<ui_adaptor> ui = make_ui_layer();

    return do_main_layer();
}

std::unique_ptr<ui_adaptor> main_menu::make_ui_layer()
{
    std::unique_ptr<ui_adaptor> ui = std::make_unique<ui_adaptor>();
    ui->on_redraw( [&]( const ui_adaptor & ) {
        print_menu( w_open, sel_main, menu_offset );

        if( layer == LAYER_MAIN ) {
            if( sel_main == LAY_MAIN_MOTD ) {
                display_text( mmenu_motd, "MOTD", text_scroll_pos );
            } else if( sel_main == LAY_MAIN_CREDITS ) {
                display_text( mmenu_credits, "Credits", text_scroll_pos );
            }
        }
        if( layer == LAYER_NEW_GAME ) {
            center_print( w_open, getmaxy( w_open ) - 7, c_yellow, hints_new_game[sel_new_game] );
            print_menu_items( w_open, entries_new_game, sel_new_game, menu_offset + point( 0, -2 ) );
        }
        if( layer == LAYER_CHOOSE_PRESET ) {
            if( templates.empty() ) {
                mvwprintz( w_open, menu_offset + point( 20 + extra_w / 2, -4 ),
                           c_red, "%s", _( "No templates found!" ) );
            } else {
                fold_and_print( w_open, menu_offset + point( 20 + extra_w / 2, -2 ), 0,
                                c_light_gray, "%s", _( "Press [<color_white>d</color>] to delete a preset." ) );
                for( int i = 0; i < static_cast<int>( templates.size() ); i++ ) {
                    int line = menu_offset.y - 4 - i;
                    mvwprintz( w_open, point( 20 + menu_offset.x + extra_w / 2, line ),
                               ( sel_preset == i ? h_white : c_white ), "%s",
                               templates[i] );
                }
            }
        }
        if( layer == LAYER_LOAD_WORLD ) {
            const point offset( 15, 0 );
            std::vector<std::string> all_worldnames = world_generator->all_worldnames();
            if( all_worldnames.empty() ) {
                mvwprintz( w_open, menu_offset + point( offset.x + extra_w / 2, -2 ),
                           c_red, "%s", _( "No Worlds found!" ) );
            } else {
                for( int i = 0; i < static_cast<int>( all_worldnames.size() ); ++i ) {
                    int line = menu_offset.y - 2 - i;
                    std::string world_name = all_worldnames[i];
                    int savegames_count = world_generator->get_world( world_name )->world_saves.size();
                    nc_color color1 = c_white;
                    nc_color color2 = h_white;
                    mvwprintz( w_open, offset + point( extra_w / 2 + menu_offset.x, line ),
                               ( sel_load_world == i ? color2 : color1 ), "%s (%d)",
                               world_name, savegames_count );
                }
            }
        }
        if( layer == LAYER_LOAD_CHARACTER ) {
            const point offset = point( 15 + extra_w / 2 + menu_offset.x, menu_offset.y - 2 - sel_load_world );

            mvwprintz( w_open, offset, h_white, "%s", selected_world );

            if( savegames.empty() ) {
                mvwprintz( w_open, offset + point( 15, 0 ), c_red, "%s", _( "No Savegames found!" ) );
            } else {
                for( size_t i = 0; i < savegames.size(); ++i ) {
                    nc_color text_color;
                    if( sel_load_character == static_cast<int>( i ) ) {
                        text_color = h_white;
                    } else {
                        text_color = c_white;
                    }
                    mvwprintz( w_open, offset + point( 15, -i ), text_color, savegames[i].player_name() );
                }
            }
        }
        if( layer == LAYER_WORLD_LIST ) {
            mvwprintz( w_open, menu_offset + point( 25 + extra_w / 2, -2 ),
                       ( sel_world_list == 0 ? h_white : c_white ), "%s", _( "Create World" ) );

            const std::vector<std::string> all_worldnames = world_generator->all_worldnames();
            for( int i = 0; i < static_cast<int>( all_worldnames.size() ); i++ ) {
                const std::string &world = all_worldnames[i];
                int savegames_count = world_generator->get_world( world )->world_saves.size();
                int line = menu_offset.y - 3 - i;
                nc_color color1 = c_white;
                nc_color color2 = h_white;
                mvwprintz( w_open, point( 25 + menu_offset.x + extra_w / 2, line ),
                           ( sel_world_list == ( i + 1 ) ? color2 : color1 ), "%s (%d)", world, savegames_count );
            }
        }
        if( layer == LAYER_WORLD_ACTION ) {
            const point offset = menu_offset + point( 40 + extra_w / 2, -2 - sel_world_list );

            mvwprintz( w_open, offset + point( -15, 0 ), h_white, "%s", selected_world );

            for( size_t i = 0; i < entries_world.size(); ++i ) {
                nc_color text_color;
                nc_color key_color;
                if( sel_world_action == static_cast<int>( i ) ) {
                    text_color = h_white;
                    key_color = h_white;
                } else {
                    text_color = c_light_gray;
                    key_color = c_white;
                }
                wmove( w_open, offset + point( 0, -i ) );
                wprintz( w_open, c_light_gray, "[" );
                shortcut_print( w_open, text_color, key_color, entries_world[i] );
                wprintz( w_open, c_light_gray, "]" );
            }
        }
        if( layer == LAYER_SPECIAL ) {
            std::vector<std::string> special_names;
            int xlen = 0;
            for( int i = 1; i < NUM_SPECIAL_GAMES; i++ ) {
                std::string spec_name = special_game_name( static_cast<special_game_id>( i ) );
                special_names.push_back( spec_name );
                xlen += utf8_width( shortcut_text( c_white, spec_name ), true ) + 2;
            }
            xlen += special_names.size() - 1;
            point offset( menu_offset + point( -( xlen / 4 ) + 32 + extra_w / 2, -2 ) );
            print_menu_items( w_open, special_names, sel_special, offset );
        }
        if( layer == LAYER_SETTINGS ) {
            std::vector<std::string> settings_subs;
            int xlen = 0;
            for( int i = 0; i < LAY_SETTINGS_NUM; ++i ) {
                settings_subs.push_back( entries_settings[i] );
                // Open and close brackets added
                xlen += utf8_width( shortcut_text( c_white, entries_settings[i] ), true ) + 2;
            }
            xlen += settings_subs.size() - 1;
            point offset = menu_offset + point( 46 + extra_w / 2 - ( xlen / 4 ), -2 );
            if( settings_subs.size() > 1 ) {
                offset.x -= 6;
            }
            print_menu_items( w_open, settings_subs, sel_settings, offset );
        }

        wnoutrefresh( w_open );
    } );
    ui->on_screen_resize( [this]( ui_adaptor & ui ) {
        init_windows();
        ui.position_from_window( w_open );
    } );
    ui->mark_resize();
    return ui;
}

bool main_menu::do_main_layer()
{
    layer = LAYER_MAIN;
    sel_main = LAY_MAIN_MOTD;
    /*
    if( !world_generator->all_worldnames().empty() ) {
        sel_main = LAY_MAIN_LOAD_GAME;
    } else {
        sel_main = LAY_MAIN_NEW_GAME;
    }
    */

    bool start = false;
    while( !start ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        std::string sInput = ctxt.get_raw_input().text;

        // switch off ime at program start
        if( ctxt.get_raw_input().sequence.empty() ) {
            // FIXME: disable_ime only seems to work after receiving an input event
            // with empty input sequence. (empty input event is also fired when the
            // window loses focus, might be related?)
            disable_ime();
            continue;
        }

        // check automatic menu shortcuts
        for( size_t i = 0; i < hotkeys_main.size(); ++i ) {
            for( const std::string &hotkey : hotkeys_main[i] ) {
                if( sInput == hotkey ) {
                    sel_main = i;
                    action = "CONFIRM";
                }
            }
        }
        // also check special keys
        if( action == "QUIT" ) {
            if( query_yn( _( "Really quit?" ) ) ) {
                sel_main = LAY_MAIN_QUIT;
                action = "CONFIRM";
            }
        } else if( move_left_right( action, sel_main, LAY_MAIN_NUM ) ) {
            text_scroll_pos = 0;
        }

        if( sel_main == LAY_MAIN_MOTD || sel_main == LAY_MAIN_CREDITS ) {
            if( action == "UP" || action == "PAGE_UP" ) {
                text_scroll_pos--;
            } else if( action == "DOWN" || action == "PAGE_DOWN" ) {
                text_scroll_pos++;
            }
        } else if( action == "UP" || action == "CONFIRM" ) {
            if( sel_main == LAY_MAIN_NEW_GAME ) {
                start = do_new_game_layer();
            } else if( sel_main == LAY_MAIN_LOAD_GAME ) {
                start = do_load_world_layer();
            } else if( sel_main == LAY_MAIN_WORLDS ) {
                do_world_list_layer();
            } else if( sel_main == LAY_MAIN_SPECIAL ) {
                start = do_special_layer();
            } else if( sel_main == LAY_MAIN_SETTINGS ) {
                do_settings_layer();
            } else if( sel_main == LAY_MAIN_HELP ) {
                get_help().display_help();
            } else if( sel_main == LAY_MAIN_QUIT ) {
                return false;
            } else {
                debugmsg( "Undefined action" );
            }
            layer = LAYER_MAIN;
        }
    }
    return true;
}

bool main_menu::do_new_game_layer()
{
    layer = LAYER_NEW_GAME;
    sel_new_game = 0;

    bool start = false;
    while( !start ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        std::string sInput = ctxt.get_raw_input().text;

        for( size_t i = 0; i < hotkeys_new_game.size(); ++i ) {
            for( const std::string &hotkey : hotkeys_new_game[i] ) {
                if( sInput == hotkey ) {
                    sel_new_game = i;
                    action = "CONFIRM";
                }
            }
        }

        if( move_left_right( action, sel_new_game, LAY_NEWGAME_NUM ) ) {
            continue;
        } else if( action == "QUIT" || action == "DOWN" ) {
            return false;
        } else if( action == "UP" || action == "CONFIRM" ) {
            if( sel_new_game == LAY_NEWGAME_PRESET ) {
                start = do_new_game_from_preset_layer();
                layer = LAYER_NEW_GAME;
            } else {
                on_out_of_scope cleanup( []() {
                    g->u = avatar();
                    world_generator->set_active_world( nullptr );
                } );
                g->gamemode = nullptr;

                bool show_prompt = sel_new_game != LAY_NEWGAME_PLAY_RANDOM &&
                                   sel_new_game != LAY_NEWGAME_PLAY_FIXED;
                WORLDPTR world = world_generator->pick_world( show_prompt );
                if( world == nullptr ) {
                    continue;
                }
                world_generator->set_active_world( world );
                try {
                    g->setup();
                } catch( const std::exception &err ) {
                    debugmsg( "Error: %s", err.what() );
                    continue;
                }
                character_type play_type = character_type::CUSTOM;
                switch( sel_new_game ) {
                    case LAY_NEWGAME_CUSTOM:
                        play_type = character_type::CUSTOM;
                        break;
                    case LAY_NEWGAME_RANDOM:
                        play_type = character_type::RANDOM;
                        break;
                    case LAY_NEWGAME_PLAY_FIXED:
                        play_type = character_type::NOW;
                        break;
                    case LAY_NEWGAME_PLAY_RANDOM:
                        play_type = character_type::FULL_RANDOM;
                        break;
                }
                if( !g->u.create( play_type ) ) {
                    load_char_templates();
                    MAPBUFFER.reset();
                    overmap_buffer.clear();
                    continue;
                }

                if( !g->start_game() ) {
                    continue;
                }
                cleanup.cancel();
                start = true;
            }
        }
    }

    if( start ) {
        g->u.add_msg_if_player( g->scen->description( g->u.male ) );

        world_generator->last_world_name = world_generator->active_world->world_name;
        world_generator->last_character_name = g->u.name;
        world_generator->save_last_world_info();
    }

    return start;
}

bool main_menu::do_new_game_from_preset_layer()
{
    layer = LAYER_CHOOSE_PRESET;
    sel_preset = 0;

    if( templates.empty() ) {
        on_error();
        clear_error();
    }

    bool start = false;
    while( !start ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        int num_presets = static_cast<int>( templates.size() );
        if( num_presets == 0 ) {
            if( action != "TIMEOUT" ) {
                return false;
            }
            continue;
        }

        if( action == "QUIT" || action == "LEFT" ) {
            return false;
        } else if( move_up_down( action, sel_preset, num_presets ) ) {
            continue;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            on_out_of_scope cleanup( []() {
                g->u = avatar();
                world_generator->set_active_world( nullptr );
            } );
            g->gamemode = nullptr;
            WORLDPTR world = world_generator->pick_world();
            if( world == nullptr ) {
                continue;
            }
            world_generator->set_active_world( world );
            try {
                g->setup();
            } catch( const std::exception &err ) {
                debugmsg( "Error: %s", err.what() );
                continue;
            }
            if( !g->u.create( character_type::TEMPLATE, templates[sel_preset] ) ) {
                load_char_templates();
                MAPBUFFER.reset();
                overmap_buffer.clear();
                continue;
            }
            if( !g->start_game() ) {
                continue;
            }
            cleanup.cancel();
            start = true;
        } else if( action == "DELETE_TEMPLATE" ) {
            if( query_yn( _( "Are you sure you want to delete %s?" ),
                          templates[sel_preset].c_str() ) ) {
                const auto path = PATH_INFO::templatedir() + templates[sel_preset] + ".template";
                if( !remove_file( path ) ) {
                    popup( _( "Sorry, something went wrong." ) );
                } else {
                    templates.erase( templates.begin() + sel_preset );
                    if( static_cast<size_t>( sel_preset ) > templates.size() - 1 ) {
                        sel_preset--;
                    }
                }
            }
        }
    }
    return start;
}

bool main_menu::do_load_world_layer()
{
    layer = LAYER_LOAD_WORLD;
    sel_load_world = 0;

    bool start = false;

    const int num_worlds = world_generator->all_worldnames().size();

    if( num_worlds == 0 ) {
        on_error();
        clear_error();
    }

    while( !start ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        if( num_worlds == 0 ) {
            if( action != "TIMEOUT" ) {
                return false;
            }
            continue;
        }

        if( move_up_down( action, sel_load_world, num_worlds ) ) {
            continue;
        } else if( action == "LEFT" || action == "QUIT" ) {
            return false;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            selected_world = world_generator->all_worldnames()[ sel_load_world ];
            start = do_load_character_layer();
            selected_world.clear();
            layer = LAYER_LOAD_WORLD;
        }
    }

    return start;
}

bool main_menu::do_load_character_layer()
{
    layer = LAYER_LOAD_CHARACTER;
    sel_load_character = 0;

    bool start = false;

    savegames = world_generator->get_world( selected_world )->world_saves;
    if( MAP_SHARING::isSharing() ) {
        auto new_end = std::remove_if( savegames.begin(), savegames.end(),
        []( const save_t &str ) {
            return str.player_name() != MAP_SHARING::getUsername();
        } );
        savegames.erase( new_end, savegames.end() );
    }

    const int num_savegames = static_cast<int>( savegames.size() );

    if( num_savegames == 0 ) {
        on_error();
        clear_error();
    }

    while( !start ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        if( num_savegames == 0 ) {
            if( action != "TIMEOUT" ) {
                return false;
            }
            continue;
        }

        if( move_up_down( action, sel_load_character, num_savegames ) ) {
            continue;
        } else if( action == "LEFT" || action == "QUIT" ) {
            return false;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            const save_t &savegame = savegames[sel_load_character];

            on_out_of_scope cleanup( []() {
                g->u = avatar();
                world_generator->set_active_world( nullptr );
            } );

            g->gamemode = nullptr;
            WORLDPTR world = world_generator->get_world( selected_world );
            world_generator->last_world_name = world->world_name;
            world_generator->last_character_name = savegame.player_name();
            world_generator->save_last_world_info();
            world_generator->set_active_world( world );

            try {
                g->setup();
            } catch( const std::exception &err ) {
                debugmsg( "Error: %s", err.what() );
                continue;
            }

            if( g->load( savegame ) ) {
                cleanup.cancel();
                start = true;
            }
        }
    }

    return start;
}

void main_menu::do_world_list_layer()
{
    if( MAP_SHARING::isSharing() && !MAP_SHARING::isWorldmenu() && !MAP_SHARING::isAdmin() ) {
        popup( _( "Only the admin can change worlds." ) );
        return;
    }

    layer = LAYER_WORLD_LIST;
    sel_world_list = 0;

    while( true ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        const int num_worlds = world_generator->all_worldnames().size();

        if( move_up_down( action, sel_world_list, num_worlds + 1 ) ) {
            continue;
        } else if( action == "LEFT" || action == "QUIT" ) {
            return;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            if( sel_world_list == 0 ) {
                world_generator->make_new_world();
            } else {
                selected_world = world_generator->all_worldnames()[ sel_world_list - 1 ];
                if( do_world_action_layer() ) {
                    sel_world_list = 0;
                }
                selected_world.clear();
                layer = LAYER_WORLD_LIST;
            }
        }
    }
}

bool main_menu::do_world_action_layer()
{
    layer = LAYER_WORLD_ACTION;
    sel_world_action = 0;

    while( true ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        if( move_up_down( action, sel_world_action, LAY_WORLDACT_NUM ) ) {
            continue;
        } else if( action == "LEFT" || action == "QUIT" ) {
            return false;
        } else if( action == "RIGHT" || action == "CONFIRM" ) {
            if( sel_world_action == LAY_WORLDACT_DELETE ) {
                if( query_yn( _( "Delete the world and all saves?" ) ) ) {
                    world_generator->delete_world( selected_world, true );
                    savegames.clear();
                    MAPBUFFER.reset();
                    overmap_buffer.clear();
                    return true;
                }
            } else if( sel_world_action == LAY_WORLDACT_RESET ) {
                if( query_yn( _( "Remove all saves and regenerate world?" ) ) ) {
                    world_generator->delete_world( selected_world, false );
                    savegames.clear();
                    MAPBUFFER.reset();
                    overmap_buffer.clear();
                    return true;
                }
            } else if( sel_world_action == LAY_WORLDACT_SHOW_MODS ) {
                WORLDPTR world_ptr = world_generator->get_world( selected_world );
                world_generator->show_active_world_mods( world_ptr->active_mod_order );
            } else if( sel_world_action == LAY_WORLDACT_EDIT_MODS ) {
                if( query_yn( _(
                                  "Editing mod list or mod load order may render the world unstable or completely unplayable.  "
                                  "It is advised to manually back up world files before proceeding.  "
                                  "If you have just started playing, consider creating new world instead.\n"
                                  "Proceed?"
                              ) ) ) {
                    WORLDPTR world_ptr = world_generator->get_world( selected_world );
                    world_generator->edit_active_world_mods( world_ptr );
                }
            } else if( sel_world_action == LAY_WORLDACT_COPY_SETTINGS ) {
                world_generator->make_new_world( true, selected_world );
                return false;
            } else if( sel_world_action == LAY_WORLDACT_MAKE_TEMPL ) {
                // FIXME
                /*
                if( do_load_world_layer( true ) ) {
                    points_left points;
                    points.stat_points = 0;
                    points.trait_points = 0;
                    points.skill_points = 0;
                    points.limit = points_left::TRANSFER;

                    g->u.setID( character_id(), true );
                    g->u.reset_all_misions();
                    g->u.save_template( g->u.name, points );

                    g->u = avatar();
                    MAPBUFFER.reset();
                    overmap_buffer.clear();

                    load_char_templates();
                }
                */
                layer = LAYER_WORLD_ACTION;
            } else {
                debugmsg( "Undefined action" );
            }
        }
    }
    return false;
}

bool main_menu::do_special_layer()
{
    // Thee can't save special games, therefore thee can't share them
    if( MAP_SHARING::isSharing() ) {
        popup( _( "Special games don't work with shared maps." ) );
        return false;
    }

    layer = LAYER_SPECIAL;
    sel_special = 0;

    while( true ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();

        if( move_left_right( action, sel_special, NUM_SPECIAL_GAMES - 1 ) ) {
            continue;
        } else if( action == "DOWN" || action == "QUIT" ) {
            return false;
        } else if( action == "UP" || action == "CONFIRM" ) {
            on_out_of_scope cleanup( []() {
                g->gamemode.reset();
                g->u = avatar();
                world_generator->set_active_world( nullptr );
            } );
            g->gamemode = get_special_game( static_cast<special_game_id>( sel2 + 1 ) );
            // check world
            WORLDPTR world = world_generator->make_new_world( static_cast<special_game_id>( sel2 + 1 ) );
            if( world == nullptr ) {
                continue;
            }
            world_generator->set_active_world( world );
            try {
                g->setup();
            } catch( const std::exception &err ) {
                debugmsg( "Error: %s", err.what() );
                continue;
            }
            if( !g->gamemode->init() ) {
                continue;
            }
            cleanup.cancel();
            return true;
        }
    }
    return false;
}

void main_menu::do_settings_layer()
{
    layer = LAYER_SETTINGS;
    sel_settings = 0;

    while( true ) {
        ui_manager::redraw();
        std::string action = ctxt.handle_input();
        std::string sInput = ctxt.get_raw_input().text;

        for( int i = 0; i < LAY_SETTINGS_NUM; ++i ) {
            for( const std::string &hotkey : hotkeys_settings[i] ) {
                if( sInput == hotkey ) {
                    sel_settings = i;
                    action = "CONFIRM";
                }
            }
        }

        if( move_left_right( action, sel_settings, LAY_SETTINGS_NUM ) ) {
            continue;
        } else if( action == "DOWN" || action == "QUIT" ) {
            return;
        } else if( action == "UP" || action == "CONFIRM" ) {
            if( sel_settings == LAY_SETTINGS_OPTIONS ) {
                get_options().show( false );
                // The language may have changed- gracefully handle this.
                init_strings();
            } else if( sel_settings == LAY_SETTINGS_KEYBINDS ) {
                input_context ctxt_default = get_default_mode_input_context();
                ctxt_default.display_menu();
            } else if( sel_settings == LAY_SETTINGS_AUTOPICKUP ) {
                get_auto_pickup().show();
            } else if( sel_settings == LAY_SETTINGS_SAFEMODE ) {
                get_safemode().show();
            } else if( sel_settings == LAY_SETTINGS_DISTRACTIONS ) {
                get_distraction_manager().show();
            } else if( sel_settings == LAY_SETTINGS_COLORS ) {
                all_colors.show_gui();
            }
        }
    }
}


std::string main_menu::halloween_spider()
{
    static const std::string spider =
        "\\ \\ \\/ / / / / / / /\n"
        " \\ \\/\\/ / / / / / /\n"
        "\\ \\/__\\/ / / / / /\n"
        " \\/____\\/ / / / /\n"
        "\\/______\\/ / / /\n"
        "/________\\/ / /\n"
        "__________\\/ /\n"
        "___________\\/\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "        |\n"
        "  , .   |  . ,\n" // NOLINT(cata-text-style)
        "  { | ,--, | }\n" // NOLINT(cata-text-style)
        "   \\\\{~~~~}//\n"
        "  /_/ {<color_c_red>..</color>} \\_\\\n"
        "  { {      } }\n"
        "  , ,      , ."; // NOLINT(cata-text-style)

    return spider;
}

std::string main_menu::halloween_graves()
{
    static const std::string graves =
        "                    _\n"
        "        -q       __(\")_\n"
        "         (\\      \\_  _/\n"
        " .-.   .-''\"'.     |/\n" // NOLINT(cata-text-style)
        "|RIP|  | RIP |   .-.\n"
        "|   |  |     |  |RIP|\n"
        ";   ;  |     | ,'---',"; // NOLINT(cata-text-style)

    return graves;
}
