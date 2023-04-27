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

#include "main.h"

using std::string;

// various options
int music_volume = 75;
int sfx_volume = 75;
bool mono_toggle = false;
int frame_cap = 120;
bool fps_toggle = false;
bool fullscreen_toggle = false;
bool true_fullscreen_toggle = false;    // note: not used in the options menu!
bool vsync_toggle = false;
bool grid_toggle = true;
bool rumble_toggle = true;
int controller_index = 0;

// flags that control option menu logic for rebinding buttons/keys
unsigned int current_rebind_index = 0;
bool rebinding_keys = false;
bool rebinding_controller = false;

enum option_id {
    MUSIC,
    SFX,
    TOGGLE_MONO,
    FULLSCREEN,
    VSYNC,
    FRAME_CAP,
    TOGGLE_FPS,
    TOGGLE_GRID,
    TOGGLE_RUMBLE,
    CONTROLLER_ID,
    REBIND_KEYBOARD,
    REBIND_CONTROLLER,
    SAVE,
    EXIT_OPTIONS,
    NONE,
};

struct option_item {
    const option_id id;
    const char* name;
    const char* description;
};

option_item options[] = {
    {MUSIC,             "Music Volume",      "Controls the volume of music."},
    {SFX,               "SFX Volume",        "Controls the volume of sound effects."},
    {TOGGLE_MONO,       "Speaker Output",    "Controls whether to output audio in mono or stereo."},
    {FULLSCREEN,        "Fullscreen",        "Sets the game's resolution to your monitor's resolution; known as 'borderless' fullscreen."},
    {VSYNC,             "V-Sync",            "Syncs the game's framerate to your monitor's refresh rate."},
    {FRAME_CAP,         "Frame Cap",         "The maximum framerate the game runs at, if V-Sync is disabled."},
    {TOGGLE_FPS,        "Display FPS",       "Shows the framerate in the top-left corner."},
    {TOGGLE_GRID,       "Display Grid",      "Controls whether to display the grid overlay during gameplay."},
    {TOGGLE_RUMBLE,     "Controller Rumble", "Controls whether to rumble the controller on every beat."},
    {CONTROLLER_ID,     "Controller Index",  "Sets which game controller to use."},
    {REBIND_KEYBOARD,   "Rebind Keyboard",   "Sets all bindings for the keyboard."},
    {REBIND_CONTROLLER, "Rebind Controller", "Sets all bindings for the controller."},
    {SAVE,              "Save Settings",     "Saves your settings and returns to the main menu."},
    {EXIT_OPTIONS,      "Exit",              "Returns to the main menu. No changes will be saved."}
};

int option_count = std::size(options);
int option_selected = 0;

const char* get_option_name(int x = option_selected) {
    return options[x].name;
}

const char* get_option_desc() {
    return options[option_selected].description;
}

int get_option_selection() {
    return option_selected;
}

int get_rebind_index() {
    return current_rebind_index;
}

void increment_rebind_index() {
    current_rebind_index++;
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
        case MUSIC: return std::to_string(music_volume).append("%");
        case SFX: return std::to_string(sfx_volume).append("%");
        case TOGGLE_MONO: return mono_toggle ? "Mono" : "Stereo";
        case FULLSCREEN: return fullscreen_toggle ? "Enabled" : "Disabled";
        case VSYNC: return vsync_toggle ? "Enabled" : "Disabled";
        case FRAME_CAP: return std::to_string(frame_cap);
        case TOGGLE_FPS: return fps_toggle ? "Enabled" : "Disabled";
        case TOGGLE_GRID: return grid_toggle ? "Enabled" : "Disabled";
        case TOGGLE_RUMBLE: return rumble_toggle ? "Enabled" : "Disabled";
        case CONTROLLER_ID: return std::to_string(controller_index);
        default: return "";
    }
}

int modify_option_value(int cur_val = 0, int mod = 1, int lower_bound = 0, int upper_bound = 100) {
    if (cur_val + mod > upper_bound) {
        return upper_bound;
    }

    if (cur_val + mod < lower_bound) {
        return lower_bound;
    }
    
    return cur_val + mod;
}

void modify_current_option_directions(int mod_value = 1) {
    // this function is what handles how each option can be interacted with via left/right/L-R buttons
    
    option_id current_selection = options[option_selected].id;
    
    switch(current_selection) {
        case MUSIC:
            music_volume = modify_option_value(music_volume, mod_value, 0, 100);
            set_music_volume();
            break;
            
        case SFX:
            sfx_volume = modify_option_value(sfx_volume, mod_value, 0, 100);
            set_sfx_volume();
            break;
            
        case FRAME_CAP:
            frame_cap = modify_option_value(frame_cap, mod_value, 30, 1000);
            set_frame_cap_ms();
            break;
            
        case CONTROLLER_ID:
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
        case TOGGLE_MONO:
            mono_toggle = !mono_toggle;
            set_channel_mix();
            break;
            
        case FULLSCREEN:
            fullscreen_toggle = !fullscreen_toggle;
            set_fullscreen();
            break;
            
        case VSYNC:
            vsync_toggle = !vsync_toggle;
            set_vsync_renderer();
            break;
            
        case TOGGLE_FPS:
            fps_toggle = !fps_toggle;
            break;
            
        case TOGGLE_GRID:
            grid_toggle = !grid_toggle;
            break;
            
        case TOGGLE_RUMBLE:
            rumble_toggle = !rumble_toggle;
            rumble_controller(500);
            break;
            
        case REBIND_KEYBOARD:
            rebinding_keys = true;
            break;
        
        case REBIND_CONTROLLER:
            rebinding_controller = true;
            break;
            
        case SAVE:
            save_settings();
            return 1;
            
        case EXIT_OPTIONS:
            return 1;
        
        default: return 0;
    }
    
    return 0;
}

void move_option_selection(int x) {
    option_selected += x;
    
    if (option_selected > option_count-1) {option_selected = 0;}
    if (option_selected < 0) {option_selected = option_count-1;}
    return;
}