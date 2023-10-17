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
#include <cmath>

#include "main.h"

using std::string;

enum tutorial_states {
    TUT_NONE,
    TUT_FACE,
    TUT_SHAPES,
    TUT_GRID_TYPE,
    TUT_GRID_MOVE,
    TUT_GRID_SIZE,
    TUT_CALL_RESP,
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
    TUT_NONE, "tutorial.message.intro",
    TUT_FACE, "tutorial.message.basic",
    TUT_SHAPES, "tutorial.message.shapes",
    TUT_GRID_TYPE, "tutorial.message.make",
    TUT_GRID_MOVE, "tutorial.message.move",
    TUT_GRID_SIZE, "tutorial.message.resize",
    TUT_NONE, "tutorial.message.timing",
    TUT_CALL_RESP, "tutorial.message.callresp",
    TUT_LIFE, "tutorial.message.life",
    TUT_FACE, "tutorial.message.outro"
};

void init_tutorial() {
    message_index = 0;
    message_tick = 0;
    message_tick_rate = 20;
    message_finished = false;
    tutorial_finished = false;
    current_message.clear();
    return;
}

void tutorial_message_tick(int frame_time) {
    message_tick -= frame_time;
    string message = get_lang_string(messages[message_index].msg);

    if (current_message.length() == message.length()) {
        message_finished = true;
        return;
    }

    if (message_tick <= 0) {
        int append_amount = fmax(abs(message_tick) / message_tick_rate, 1);

        current_message.append(message, current_message.length(), append_amount);
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
            message_tick_rate = 20;
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