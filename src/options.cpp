/*  Open Manifold source file
*
*   This program/source code is licensed under the MIT License:
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*/

#include <cstdlib>
#include <string>
#include <vector>

#include "main.h"

using std::string;
using std::vector;

// various options
int music_volume = 75;
int sfx_volume = 75;
bool mono_toggle = false;
int frame_cap = 120;
bool fps_toggle = false;
bool fullscreen_toggle = false;
bool true_fullscreen_toggle = false; // note: not used in the options menu!
bool vsync_toggle = false;
bool blindfold_toggle = false;
bool grid_toggle = true;
bool rumble_toggle = true;
int controller_index = 0;

// flags that control option menu logic for rebinding buttons/keys
unsigned int current_rebind_index = 0;
bool rebinding_keys = false;
bool rebinding_controller = false;

enum option_id {
    OPT_SUB_VIDEO,
    OPT_SUB_AUDIO,
    OPT_SUB_CONTROLS,
    OPT_SUB_GAMEPLAY,
    OPT_SUB_MISC,
    OPT_MUSIC,
    OPT_SFX,
    OPT_TOGGLE_MONO,
    OPT_FULLSCREEN,
    OPT_VSYNC,
    OPT_FRAME_CAP,
    OPT_TOGGLE_FPS,
    OPT_TOGGLE_GRID,
    OPT_TOGGLE_BLINDFOLD,
    OPT_TOGGLE_RUMBLE,
    OPT_CONTROLLER_ID,
    OPT_REBIND_KEYBOARD,
    OPT_REBIND_CONTROLLER,
    OPT_RESET_KEYBOARD,
    OPT_RESET_CONTROLLER,
    OPT_SAVE,
    OPT_EXIT,
    OPT_BACK,
    OPT_NONE,
};

struct option_item {
    option_id id;
    const char* name = "";
    const char* description = "";
};

option_item option_back = {
    OPT_BACK,
    "Back",
    "Return to the main options menu"
};

vector<option_item> options_main = {
    {OPT_SUB_VIDEO,     "Video Settings",       "Change graphics settings here."},
    {OPT_SUB_AUDIO,     "Audio Settings",       "Change audio settings here."},
    {OPT_SUB_CONTROLS,  "Controller Settings",  "Change controller settings here."},
    {OPT_SUB_GAMEPLAY,  "Gameplay Settings",    "Change gameplay settings here."},
    // {OPT_SUB_MISC,      "Other Settings",       "Change other settings here."},
    {OPT_NONE},
    {OPT_SAVE,          "Save Settings",        "Saves your settings and returns to the main menu."},
    {OPT_EXIT,          "Exit",                 "Returns to the main menu. No changes will be saved."}
};

vector<option_item> options_video = {
    {OPT_FULLSCREEN,    "Fullscreen",   "Sets the game's resolution to your monitor resolution."},
    {OPT_VSYNC,         "V-Sync",       "Syncs the game's video output to your monitor refresh rate."},
    {OPT_FRAME_CAP,     "Frame Cap",    "The max framerate the game runs at when V-Sync is off."},
    {OPT_TOGGLE_FPS,    "Display FPS",  "Shows the frames-per-second and frame time."},
    {OPT_NONE},
    option_back
};

vector<option_item> options_audio = {
    {OPT_MUSIC,         "Music Volume",     "Controls the volume of music."},
    {OPT_SFX,           "SFX Volume",       "Controls the volume of sound effects."},
    {OPT_TOGGLE_MONO,   "Speaker Output",   "Controls the number of audio channels to output to."},
    {OPT_NONE},
    option_back
};

vector<option_item> options_controls = {
    {OPT_REBIND_KEYBOARD,   "Rebind Keyboard",          "Sets all bindings for the keyboard."},
    {OPT_REBIND_CONTROLLER, "Rebind Controller",        "Sets all bindings for the controller."},
    {OPT_TOGGLE_RUMBLE,     "Controller Rumble",        "Rumbles the controller on every other beat."},
    {OPT_CONTROLLER_ID,     "Controller Index",         "Sets which game controller to use."},
    {OPT_NONE},
    {OPT_RESET_KEYBOARD,    "Reset Keyboard Binds",     "Resets all bindings for the keyboard."},
    {OPT_RESET_CONTROLLER,  "Reset Controller Binds",   "Resets all bindings for the controller."},
    {OPT_NONE},
    option_back
};

vector<option_item> options_gameplay = {
    {OPT_TOGGLE_GRID,       "Display Grid",      "Displays the shape grid overlay."},
    {OPT_TOGGLE_BLINDFOLD,  "Blindfold Mode",    "Makes all placed and player-controlled shapes invisible."},
    {OPT_NONE},
    option_back
};

option_id option_submenu_id = OPT_NONE;

vector<option_item> options = options_main;

int option_selected = 0;

const char* get_option_name(int x = option_selected) {
    return options[x].name;
}

const char* get_option_desc() {
    return options[option_selected].description;
}

int get_option_count() {
    return std::size(options);
}

int get_option_selection() {
    return option_selected;
}

int get_rebind_index() {
    return current_rebind_index;
}

void set_option_menu(option_id id) {
    option_selected = 0;
    option_submenu_id = id;

    switch (option_submenu_id) {
        case OPT_SUB_VIDEO:
            options = options_video;
            break;

        case OPT_SUB_AUDIO:
            options = options_audio;
            break;

        case OPT_SUB_CONTROLS:
            options = options_controls;
            break;

        case OPT_SUB_GAMEPLAY:
            options = options_gameplay;
            break;

        /*
        case OPT_SUB_MISC:
            options = options_misc;
            break;
        */

        case OPT_BACK:
        case OPT_NONE:
            option_submenu_id = OPT_NONE;
            options = options_main;
            break;

        default:
            break;
    }
    return;
}

void increment_rebind_index() {
    current_rebind_index++;
    return;
}

void reset_options_menu() {
    option_selected = 0;
    set_option_menu(OPT_NONE);
    return;
}

void reset_rebind_flags() {
    current_rebind_index = 0;
    rebinding_keys = false;
    rebinding_controller = false;
    return;
}

bool check_rebind_keys() {
    return rebinding_keys;
}

bool check_rebind_controller() {
    return rebinding_controller;
}

bool check_rebind() {
    return rebinding_keys || rebinding_controller;
}

string get_option_value(int index) {
    option_id id = options[index].id;

    switch (id) {
        case OPT_MUSIC: return std::to_string(music_volume).append("%");
        case OPT_SFX: return std::to_string(sfx_volume).append("%");
        case OPT_TOGGLE_MONO: return mono_toggle ? "Mono" : "Stereo";
        case OPT_FULLSCREEN: return fullscreen_toggle ? "Enabled" : "Disabled";
        case OPT_VSYNC: return vsync_toggle ? "Enabled" : "Disabled";
        case OPT_FRAME_CAP: return std::to_string(frame_cap);
        case OPT_TOGGLE_FPS: return fps_toggle ? "Enabled" : "Disabled";
        case OPT_TOGGLE_GRID: return grid_toggle ? "Enabled" : "Disabled";
        case OPT_TOGGLE_BLINDFOLD: return blindfold_toggle ? "Enabled" : "Disabled";
        case OPT_TOGGLE_RUMBLE: return rumble_toggle ? "Enabled" : "Disabled";
        case OPT_CONTROLLER_ID: return std::to_string(controller_index);
        default: return "";
    }
}

int modify_option_value(int cur_val = 0, int mod_value = 1, int lower_bound = 0, int upper_bound = 100) {
    if (cur_val + mod_value > upper_bound) {
        return upper_bound;
    }

    if (cur_val + mod_value < lower_bound) {
        return lower_bound;
    }

    return cur_val + mod_value;
}

void modify_current_option_directions(int mod_value = 1) {
    // this function is what handles how each option can be interacted with via left/right/L-R buttons

    option_id current_selection = options[option_selected].id;

    switch(current_selection) {
        case OPT_MUSIC:
            music_volume = modify_option_value(music_volume, mod_value, 0, 100);
            set_music_volume();
            break;

        case OPT_SFX:
            sfx_volume = modify_option_value(sfx_volume, mod_value, 0, 100);
            set_sfx_volume();
            break;

        case OPT_FRAME_CAP:
            frame_cap = modify_option_value(frame_cap, mod_value, 30, 1000);
            set_frame_cap_ms();
            break;

        case OPT_CONTROLLER_ID:
            controller_index = modify_option_value(controller_index, mod_value, 0, get_controller_count());
            init_controller();
            break;

        default: return;
    }

    return;
}

int modify_current_option_button() {
    // this function is what handles how each option can be interacted with via the A button
    // returns 0 normally, but if 1 is returned the game will return to the menu
    // this strange coding is because of how transition states are scoped to the main() loop, see main.cpp

    option_id current_selection = options[option_selected].id;

    switch(current_selection) {
        case OPT_SUB_VIDEO:
        case OPT_SUB_AUDIO:
        case OPT_SUB_CONTROLS:
        case OPT_SUB_GAMEPLAY:
        case OPT_SUB_MISC:
        case OPT_BACK:
            set_option_menu(current_selection);
            break;

        case OPT_TOGGLE_MONO:
            mono_toggle = !mono_toggle;
            set_channel_mix();
            play_channel_test();
            break;

        case OPT_FULLSCREEN:
            fullscreen_toggle = !fullscreen_toggle;
            set_fullscreen();
            break;

        case OPT_VSYNC:
            vsync_toggle = !vsync_toggle;
            set_vsync_renderer();
            break;

        case OPT_TOGGLE_FPS:
            fps_toggle = !fps_toggle;
            break;

        case OPT_TOGGLE_GRID:
            grid_toggle = !grid_toggle;
            break;

        case OPT_TOGGLE_BLINDFOLD:
            blindfold_toggle = !blindfold_toggle;
            break;

        case OPT_TOGGLE_RUMBLE:
            rumble_toggle = !rumble_toggle;
            rumble_controller(500);
            break;

        case OPT_REBIND_KEYBOARD:
            rebinding_keys = true;
            break;

        case OPT_REBIND_CONTROLLER:
            rebinding_controller = true;
            break;

        case OPT_RESET_KEYBOARD:
            reset_keyboard_binds();
            break;

        case OPT_RESET_CONTROLLER:
            reset_controller_binds();
            break;

        case OPT_SAVE:
            save_settings();
            return 1;

        case OPT_EXIT:
            return 1;

        default: return 0;
    }

    return 0;
}

int options_back_button() {
    if (option_submenu_id == OPT_NONE) {return 1;} else {reset_options_menu(); return 0;}
}

void move_option_selection(int x) {
    int option_count = get_option_count();
    option_selected += x;

    while (options[option_selected].id == OPT_NONE) {
        option_selected += x;
    }

    if (option_selected > option_count-1) {option_selected = 0;}
    if (option_selected < 0) {option_selected = option_count-1;}
    return;
}