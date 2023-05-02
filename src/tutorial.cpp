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

enum tutorial_states {
    TUT_NONE,
    TUT_FACE,
    TUT_SHAPES,
    TUT_GRID_TYPE,
    TUT_GRID_MOVE,
    TUT_GRID_SIZE,
    TUT_LIFE
};

int message_index = 0;
int message_tick = 0;
int message_tick_rate = 30;
bool message_finished = false;
bool tutorial_finished = false;
string current_message;

struct {
    tutorial_states state;
    string msg;
} messages[] = {
    TUT_NONE, "Welcome to Open Manifold! In this guide, we will walk through the basics of playing the game.",
    TUT_FACE, "Open Manifold is a rhythm game where the goal is to create patterns called 'faces'.",
    TUT_SHAPES, "To make faces, you create and manipulate shapes. There are three kinds of shapes: circles, squares, and triangles.",
    TUT_GRID_TYPE, "To create a shape, press one of the three face buttons. Each button corresponds to one shape.",
    TUT_GRID_MOVE, "You can freely move the shape's position along the grid with the directional buttons. ",
    TUT_GRID_SIZE, "You can also resize the shape with the shoulder buttons. The shape can be resized anywhere, even at the edges of the grid.",
    TUT_NONE, "Your actions must be timed to the beat of the song. If your input timing isn't on-beat, then nothing will happen. You only get so many beats to work with, so make 'em count!",
    TUT_NONE, "Levels play out in a call-and-response fashion. First the computer will create a shape and move it into position, and then you must replicate that shape.",
    TUT_LIFE, "You also have a lifebar. Fail to replicate a shape, and you'll lose some life. Complete a shape, and you'll get some of it back. If it hits zero, that's a game over!",
    TUT_FACE, "That should cover the basics of play. Have fun, and enjoy playing Open Manifold!"
};

void init_tutorial() {
    message_index = 0;
    message_tick = 0;
    message_tick_rate = 30;
    message_finished = false;
    tutorial_finished = false;
    current_message.clear();
    return;
}

void tutorial_message_tick(int frame_time) {
    message_tick -= frame_time;
    
    if (current_message.length() == messages[message_index].msg.length()) {
        message_finished = true;
        return;
    }
    
    if (message_tick <= 0) {
        current_message.append(messages[message_index].msg, current_message.length(), 1);
        message_tick = message_tick_rate;
        
        if (current_message.length() % 4 == 0) {
            play_dialog_blip();
        }
    }
    
    return;
}

void tutorial_advance_message() {
    message_tick_rate = 5;
    
    if (message_finished && !tutorial_finished) {
        play_dialog_advance();
        
        if (message_index >= std::size(messages) - 1) {
            tutorial_finished = true;
        } else {
            current_message.clear();
            message_index++;
            message_tick_rate = 30;
            message_tick = message_tick_rate;
            message_finished = false;
        }
    }
    
    return;
}

string get_tutorial_current_message() {
    return current_message;
}

tutorial_states get_tutorial_state() {
    return messages[message_index].state;
}

bool check_tutorial_finished() {
    return tutorial_finished;
}