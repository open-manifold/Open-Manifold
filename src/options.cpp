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
using std::to_string;
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
bool hud_toggle = true;
bool rumble_toggle = true;
int controller_index = 0;

// flags that control option menu logic for rebinding buttons/keys
unsigned int current_rebind_index = 0;
bool rebinding_keys = false;
bool rebinding_controller = false;
bool rebinding_single = false;

enum option_id {
    OPT_SUB_VIDEO,
    OPT_SUB_AUDIO,
    OPT_SUB_CONTROLS,
    OPT_SUB_KEYBOARD,
    OPT_SUB_CONTROLLER,
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
    OPT_TOGGLE_HUD,
    OPT_TOGGLE_BLINDFOLD,
    OPT_TOGGLE_RUMBLE,
    OPT_CONTROLLER_ID,
    OPT_REBIND_KEYBOARD,
    OPT_REBIND_CONTROLLER,
    OPT_RESET_KEYBOARD,
    OPT_RESET_CONTROLLER,
    OPT_REBIND_KB_UP,
    OPT_REBIND_KB_DOWN,
    OPT_REBIND_KB_LEFT,
    OPT_REBIND_KB_RIGHT,
    OPT_REBIND_KB_CROSS,
    OPT_REBIND_KB_CIRCLE,
    OPT_REBIND_KB_SQUARE,
    OPT_REBIND_KB_TRIANGLE,
    OPT_REBIND_KB_LB,
    OPT_REBIND_KB_RB,
    OPT_REBIND_KB_START,
    OPT_REBIND_KB_BACK,
    OPT_REBIND_CTRL_UP,
    OPT_REBIND_CTRL_DOWN,
    OPT_REBIND_CTRL_LEFT,
    OPT_REBIND_CTRL_RIGHT,
    OPT_REBIND_CTRL_CROSS,
    OPT_REBIND_CTRL_CIRCLE,
    OPT_REBIND_CTRL_SQUARE,
    OPT_REBIND_CTRL_TRIANGLE,
    OPT_REBIND_CTRL_LB,
    OPT_REBIND_CTRL_RB,
    OPT_REBIND_CTRL_START,
    OPT_REBIND_CTRL_BACK,
    OPT_SAVE,
    OPT_EXIT,
    OPT_BACK,
    OPT_CONTROLS_BACK,
    OPT_NONE,
};

struct option_item {
    option_id id;
    string name = "";
    string description = "";
};

option_item option_back = {
    OPT_BACK,
    "options.back",
    "options.back.desc"
};

vector<option_item> options_main = {
    {OPT_SUB_VIDEO,     "options.video",       "options.video.desc"},
    {OPT_SUB_AUDIO,     "options.audio",       "options.audio.desc"},
    {OPT_SUB_CONTROLS,  "options.controls",    "options.controls.desc"},
    {OPT_SUB_GAMEPLAY,  "options.gameplay",    "options.gameplay.desc"},
    // {OPT_SUB_MISC,      "options.misc",        "options.misc.desc"},
    {OPT_NONE},
    {OPT_SAVE,          "options.save",        "options.save.desc"},
    {OPT_EXIT,          "options.exit",        "options.exit.desc"}
};

vector<option_item> options_video = {
    {OPT_FULLSCREEN,    "options.video.fullscreen", "options.video.fullscreen.desc"},
    {OPT_VSYNC,         "options.video.vsync",      "options.video.vsync.desc"},
    {OPT_FRAME_CAP,     "options.video.framecap",   "options.video.framecap.desc"},
    {OPT_TOGGLE_FPS,    "options.video.fps",        "options.video.fps.desc"},
    {OPT_NONE},
    option_back
};

vector<option_item> options_audio = {
    {OPT_MUSIC,         "options.audio.music",     "options.audio.music.desc"},
    {OPT_SFX,           "options.audio.sfx",       "options.audio.sfx.desc"},
    {OPT_TOGGLE_MONO,   "options.audio.speaker",   "options.audio.speaker.desc"},
    {OPT_NONE},
    option_back
};

vector<option_item> options_controls = {
    {OPT_SUB_KEYBOARD,      "options.controls.rebind.kb",   "options.controls.rebind.kb.desc"},
    {OPT_SUB_CONTROLLER,    "options.controls.rebind.ctrl", "options.controls.rebind.ctrl.desc"},
    {OPT_TOGGLE_RUMBLE,     "options.controls.rumble",      "options.controls.rumble.desc"},
    {OPT_CONTROLLER_ID,     "options.controls.ctrlindex",   "options.controls.ctrlindex.desc"},
    {OPT_NONE},
    {OPT_RESET_KEYBOARD,    "options.controls.reset.kb",    "options.controls.reset.kb.desc"},
    {OPT_RESET_CONTROLLER,  "options.controls.reset.ctrl",  "options.controls.reset.ctrl.desc"},
    {OPT_NONE},
    option_back
};

vector<option_item> options_controls_kb = {
    {OPT_REBIND_KB_UP,          "button.0",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_DOWN,        "button.1",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_LEFT,        "button.2",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_RIGHT,       "button.3",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_CROSS,       "button.4",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_CIRCLE,      "button.5",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_SQUARE,      "button.6",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_TRIANGLE,    "button.7",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_LB,          "button.8",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_RB,          "button.9",             "options.rebind.kb.desc"},
    {OPT_REBIND_KB_START,       "button.10",            "options.rebind.kb.desc"},
    {OPT_REBIND_KB_BACK,        "button.11",            "options.rebind.kb.desc"},
    {OPT_NONE},
    {OPT_REBIND_KEYBOARD,       "options.rebindall",    "options.rebindall.kb.desc"},
    {OPT_CONTROLS_BACK,         "options.ctrl.back",    "options.ctrl.back.desc"}
};

vector<option_item> options_controls_ctrl = {
    {OPT_REBIND_CTRL_UP,        "button.0",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_DOWN,      "button.1",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_LEFT,      "button.2",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_RIGHT,     "button.3",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_CROSS,     "button.4",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_CIRCLE,    "button.5",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_SQUARE,    "button.6",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_TRIANGLE,  "button.7",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_LB,        "button.8",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_RB,        "button.9",         "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_START,     "button.10",        "options.rebind.ctrl.desc"},
    {OPT_REBIND_CTRL_BACK,      "button.11",        "options.rebind.ctrl.desc"},
    {OPT_NONE},
    {OPT_REBIND_CONTROLLER,     "options.rebindall",    "options.rebindall.ctrl.desc"},
    {OPT_CONTROLS_BACK,         "options.ctrl.back",    "options.ctrl.back.desc"}
};

vector<option_item> options_gameplay = {
    {OPT_TOGGLE_GRID,       "options.gameplay.grid",      "options.gameplay.grid.desc"},
    {OPT_TOGGLE_HUD,        "options.gameplay.hud",       "options.gameplay.hud.desc"},
    {OPT_TOGGLE_BLINDFOLD,  "options.gameplay.blindfold", "options.gameplay.blindfold.desc"},
    {OPT_NONE},
    option_back
};

option_id option_submenu_id = OPT_NONE;

vector<option_item> options = options_main;

int option_selected = 0;

string get_option_name(int x = option_selected) {
    return get_lang_string(options[x].name);
}

string get_option_desc() {
    return get_lang_string(options[option_selected].description);
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

        case OPT_SUB_KEYBOARD:
            options = options_controls_kb;
            break;

        case OPT_SUB_CONTROLLER:
            options = options_controls_ctrl;
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

        case OPT_CONTROLS_BACK:
            option_submenu_id = OPT_SUB_CONTROLS;
            options = options_controls;
            break;

        default:
            break;
    }
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
    rebinding_single = false;
    return;
}

void increment_rebind_index() {
    current_rebind_index++;
    if (rebinding_single) {reset_rebind_flags();}
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
    string on = get_lang_string("options.on");
    string off = get_lang_string("options.off");

    switch (id) {
        case OPT_MUSIC: return to_string(music_volume).append("%");
        case OPT_SFX: return to_string(sfx_volume).append("%");
        case OPT_TOGGLE_MONO: return mono_toggle ? get_lang_string("options.audio.speaker.mono") : get_lang_string("options.audio.speaker.stereo");
        case OPT_FULLSCREEN: return fullscreen_toggle ? on : off;
        case OPT_VSYNC: return vsync_toggle ? on : off;
        case OPT_FRAME_CAP: return to_string(frame_cap);
        case OPT_TOGGLE_FPS: return fps_toggle ? on : off;
        case OPT_TOGGLE_GRID: return grid_toggle ? on : off;
        case OPT_TOGGLE_HUD: return hud_toggle ? on : off;
        case OPT_TOGGLE_BLINDFOLD: return blindfold_toggle ? on : off;
        case OPT_TOGGLE_RUMBLE: return rumble_toggle ? on : off;
        case OPT_CONTROLLER_ID: return to_string(controller_index);
        case OPT_REBIND_KB_UP: return get_current_mapping_explicit(true, 0);
        case OPT_REBIND_KB_DOWN: return get_current_mapping_explicit(true, 1);
        case OPT_REBIND_KB_LEFT: return get_current_mapping_explicit(true, 2);
        case OPT_REBIND_KB_RIGHT: return get_current_mapping_explicit(true, 3);
        case OPT_REBIND_KB_CROSS: return get_current_mapping_explicit(true, 4);
        case OPT_REBIND_KB_CIRCLE: return get_current_mapping_explicit(true, 5);
        case OPT_REBIND_KB_SQUARE: return get_current_mapping_explicit(true, 6);
        case OPT_REBIND_KB_TRIANGLE: return get_current_mapping_explicit(true, 7);
        case OPT_REBIND_KB_LB: return get_current_mapping_explicit(true, 8);
        case OPT_REBIND_KB_RB: return get_current_mapping_explicit(true, 9);
        case OPT_REBIND_KB_START: return get_current_mapping_explicit(true, 10);
        case OPT_REBIND_KB_BACK: return get_current_mapping_explicit(true, 11);
        case OPT_REBIND_CTRL_UP: return get_current_mapping_explicit(false, 0);
        case OPT_REBIND_CTRL_DOWN: return get_current_mapping_explicit(false, 1);
        case OPT_REBIND_CTRL_LEFT: return get_current_mapping_explicit(false, 2);
        case OPT_REBIND_CTRL_RIGHT: return get_current_mapping_explicit(false, 3);
        case OPT_REBIND_CTRL_CROSS: return get_current_mapping_explicit(false, 4);
        case OPT_REBIND_CTRL_CIRCLE: return get_current_mapping_explicit(false, 5);
        case OPT_REBIND_CTRL_SQUARE: return get_current_mapping_explicit(false, 6);
        case OPT_REBIND_CTRL_TRIANGLE: return get_current_mapping_explicit(false, 7);
        case OPT_REBIND_CTRL_LB: return get_current_mapping_explicit(false, 8);
        case OPT_REBIND_CTRL_RB: return get_current_mapping_explicit(false, 9);
        case OPT_REBIND_CTRL_START: return get_current_mapping_explicit(false, 10);
        case OPT_REBIND_CTRL_BACK: return get_current_mapping_explicit(false, 11);
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
        case OPT_SUB_CONTROLLER:
        case OPT_SUB_KEYBOARD:
        case OPT_SUB_GAMEPLAY:
        case OPT_SUB_MISC:
        case OPT_BACK:
        case OPT_CONTROLS_BACK:
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

        case OPT_TOGGLE_HUD:
            hud_toggle = !hud_toggle;
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

        // lord forgive me for what im about to program
        case OPT_REBIND_KB_UP:
            current_rebind_index = 0;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_DOWN:
            current_rebind_index = 1;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_LEFT:
            current_rebind_index = 2;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_RIGHT:
            current_rebind_index = 3;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_CROSS:
            current_rebind_index = 4;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_CIRCLE:
            current_rebind_index = 5;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_SQUARE:
            current_rebind_index = 6;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_TRIANGLE:
            current_rebind_index = 7;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_LB:
            current_rebind_index = 8;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_RB:
            current_rebind_index = 9;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_START:
            current_rebind_index = 10;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_KB_BACK:
            current_rebind_index = 11;
            rebinding_keys = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_UP:
            current_rebind_index = 0;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_DOWN:
            current_rebind_index = 1;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_LEFT:
            current_rebind_index = 2;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_RIGHT:
            current_rebind_index = 3;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_CROSS:
            current_rebind_index = 4;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_CIRCLE:
            current_rebind_index = 5;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_SQUARE:
            current_rebind_index = 6;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_TRIANGLE:
            current_rebind_index = 7;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_LB:
            current_rebind_index = 8;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_RB:
            current_rebind_index = 9;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_START:
            current_rebind_index = 10;
            rebinding_controller = true;
            rebinding_single = true;
            break;

        case OPT_REBIND_CTRL_BACK:
            current_rebind_index = 11;
            rebinding_controller = true;
            rebinding_single = true;
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
    if (option_submenu_id == OPT_NONE) {return 1;} else {
        if (option_submenu_id == OPT_SUB_CONTROLLER || option_submenu_id == OPT_SUB_KEYBOARD) {
            set_option_menu(OPT_CONTROLS_BACK);
        } else reset_options_menu();
    }
    return 0;
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