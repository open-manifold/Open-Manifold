#pragma once

enum option_id {
    OPT_MUSIC,
    OPT_SFX,
    OPT_TOGGLE_MONO,
    OPT_FULLSCREEN,
    OPT_VSYNC,
    OPT_FRAME_CAP,
    OPT_TOGGLE_FPS,
    OPT_TOGGLE_GRID,
    OPT_TOGGLE_RUMBLE,
    OPT_CONTROLLER_ID,
    OPT_SAVE,
    OPT_EXIT
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
