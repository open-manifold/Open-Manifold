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

using std::string;

int message_index = 0;
int message_tick = 0;
bool message_finished = false;
string current_message;

string messages[] = {
    "Message 1",
    "Massage Two",
    "Messenger Tres",
    "Mortgage 4our",
    "Fifth set of informative combination of words for your reading entertainment or education or hopefully something along those lines anyways because these things take quite a bit of effort to write and definitely isn't just padding for the sake of having a long message to test this function with",
    "Okay but really these are placeholder lines",
    "This mode is nowhere near finished so come back later maybe xoxo love you <3"
};

void init_tutorial() {
    message_index = 0;
    message_tick = 0;
    message_finished = false;
    current_message.clear();
    return;
}

void tutorial_message_tick(int frame_time) {
    message_tick -= frame_time;
    
    if (current_message.length() == messages[message_index].length()) {
        message_finished = true;
        return;
    }
    
    if (message_tick <= 0) {
        current_message.append(messages[message_index], current_message.length(), 1);
        message_tick = 40;
    }
    
    return;
}

void tutorial_advance_message() {
    if (message_finished && message_index < std::size(messages) - 1) {
        current_message.clear();
        message_index++;
        message_tick = 40;
        message_finished = false;
    }
    
    return;
}

string get_tutorial_current_message() {
    return current_message;
}