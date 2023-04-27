#pragma once

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
    SAVE,
    EXIT_OPTIONS
};

int get_option_selection();
const char* get_option_name(int);
const char* get_option_desc();
std::string get_option_value(int);
int get_rebind_index();
void increment_rebind_index();
bool check_rebind();
bool check_rebind_keys();
bool check_rebind_controller();
void reset_rebind_flags();

void move_option_selection(int);
void modify_current_option_directions(int);
int modify_current_option_button();
