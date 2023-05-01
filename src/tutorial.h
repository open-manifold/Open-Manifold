#pragma once 

void init_tutorial();
void tutorial_message_tick(int);
void tutorial_advance_message();
std::string get_tutorial_current_message();

enum tutorial_states {
    TUT_NONE,
    TUT_FACE,
    TUT_SHAPES,
    TUT_GRID_MOVE,
    TUT_GRID_SIZE,
    TUT_LIFEBAR
};