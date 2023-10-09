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

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>

#include "graphics.h"
#include "character.h"
#include "options.h"
#include "tutorial.h"
#include "version.h"

using nlohmann::json;
using std::string;
using std::to_string;
using std::vector;

// basic window stuff
SDL_Window* window;
SDL_Renderer* renderer;
extern int width;
extern int height;

// various options
extern int sandbox_item_count;
extern int music_volume;
extern int sfx_volume;
extern bool mono_toggle;
extern int frame_cap;
extern bool fps_toggle;
extern bool fullscreen_toggle;
extern bool true_fullscreen_toggle;
extern bool vsync_toggle;
extern bool grid_toggle;
extern bool hud_toggle;
extern bool blindfold_toggle;
extern bool rumble_toggle;
extern int controller_index;
bool debug_toggle;

// main-game variables
int score = 0;
int combo = 0;
int life = 100;

// metadata storage for level select
struct {
    unsigned int hiscore;
    unsigned int play_count;
    bool cleared;
} metadata  = {0, 0, false};

// BG flags
// set to true for one frame when applicable
bool beat_advanced = false;
bool shape_advanced = false;

// timekeeping flags (and some other data)
float beat_start_time;
float length;
int bpm = 120;
int beat_count = 0;
int song_beat_position = 0;
int song_start_time;
int intro_beat_length;
string cpu_sequence;
string player_sequence;
bool song_over = false;
bool game_over = false;

// used only in main(); stored globally so it can be modified by options.cpp
int frame_cap_ms = (1000 / frame_cap);

// sound effects
Mix_Chunk *snd_menu_move;
Mix_Chunk *snd_menu_confirm;
Mix_Chunk *snd_menu_back;
Mix_Chunk *snd_mono_test;
Mix_Chunk *snd_metronome_small;
Mix_Chunk *snd_metronome_big;
Mix_Chunk *snd_up;
Mix_Chunk *snd_down;
Mix_Chunk *snd_left;
Mix_Chunk *snd_right;
Mix_Chunk *snd_circle;
Mix_Chunk *snd_square;
Mix_Chunk *snd_triangle;
Mix_Chunk *snd_xplode;
Mix_Chunk *snd_scale_up;
Mix_Chunk *snd_scale_down;
Mix_Chunk *snd_success;
Mix_Chunk *snd_combo;

// the music track that's playing
Mix_Music *music;

// in addition to actually drawing the fade, these are also used to handle game-state transitions
extern float fade_in;
extern float fade_out;

// stores file paths
vector<string> level_paths;   // e.g. "assets/levels/Foo Bar"
vector<string> level_playlists; // stores playlist names corresponding to level paths, TODO: merge this and level_paths into a single struct
int level_index = 0;

// contains the currently-loaded JSON level data
json json_file;

// stores current list of shapes and active shape for main game
// previous_shapes is all the previous shapes in order
// active shape is the one the CPU/player manipulates
// result_shape is compared to at the end of a measure to see if a shape matches
vector<shape> previous_shapes;
shape active_shape;
shape result_shape;

// all the possible game states
enum game_states {
    WARNING,
    TITLE,
    LEVEL_SELECT,
    GAME,
    SANDBOX,
    TUTORIAL,
    OPTIONS,
    EXIT,
};

// abstract controller, modelled after the original PS1 controller
// this makes it easier to support controllers as well as keyboard
enum controller_buttons {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    CROSS,
    CIRCLE,
    SQUARE,
    TRIANGLE,
    LB,
    RB,
    START,
    SELECT,
};

// array that contains SDL keyboard keymap
// (this should correspond to controller_buttons in terms of order)
SDL_Keycode keymap[12] = {
    SDLK_UP,
    SDLK_DOWN,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_z,
    SDLK_x,
    SDLK_c,
    SDLK_v,
    SDLK_a,
    SDLK_s,
    SDLK_RETURN,
    SDLK_BACKSPACE
};

SDL_GameControllerButton buttonmap[12] = {
    SDL_CONTROLLER_BUTTON_DPAD_UP,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_X,
    SDL_CONTROLLER_BUTTON_Y,
    SDL_CONTROLLER_BUTTON_A,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
    SDL_CONTROLLER_BUTTON_START,
    SDL_CONTROLLER_BUTTON_BACK
};

// the controller, if one is needed
SDL_GameController *controller;

// message of the day string
string motd = "";

bool parse_option(char** argc, char** argv, const string& opt) {
    return std::find(argc, argv, opt) != argv;
}

/**
 * Returns a pointer to the string of the argument that follows opt. If no argument
 * follows it (ie if the flag or value is not provided) it returns `nullptr`.
 */
char *parse_option_value(char **argc, char **argv, const string& opt) {
    char **flag = std::find(argc, argv, opt);
    // Check for `argv - 1` (missing argument) and also `argv` (missing flag) in the same check.
    if (flag >= argv - 1) {
        return nullptr;
    } else {
        return *(flag + 1);
    }
}

string get_version_string() {
    return "v" + to_string(VERSION_MAJOR) + "." + to_string(VERSION_MINOR) + "." + to_string(VERSION_PATCH);
}

void print_header() {
    printf("OPEN MANIFOLD %s\nBuild Date: %s at %s\n========================================\n", get_version_string().c_str(), __DATE__, __TIME__);
    return;
}

void print_help() {
    print_header();
    printf("\nAccepted parameters are:\n\n"
    "-h  / -help            - Print this message\n"
    "-l  / -log             - Writes a log to file\n"
    "-sb / -sandbox         - Start in sandbox mode\n"
    "-f  / -fullscreen      - Enable fullscreen\n"
    "-tf / -true-fullscreen - Enable 'real' fullscreen\n"
    "-v  / -vsync           - Enable V-Sync\n"
    "-d  / -debug           - Enable debug features\n"
    "-i [FOLDER PATH]       - Specify a level folder to play on start\n");
    return;
}

void set_fullscreen() {
    if (fullscreen_toggle) {SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);} else {SDL_SetWindowFullscreen(window, 0);}
    return;
}

void set_frame_cap_ms() {
    frame_cap_ms = (1000 / frame_cap);
    return;
}

void save_settings() {
    nlohmann::ordered_json new_config;

    // loads existing config.json to check for hidden settings
    std::ifstream ifs("config.json");

    if (ifs.good()) {
        try {
            json json_data = json::parse(ifs);
            if (json_data.contains("game_width")) {new_config["game_width"] = json_data["game_width"];}
            if (json_data.contains("game_height")) {new_config["game_height"] = json_data["game_height"];}
            if (json_data.contains("debug")) {new_config["debug"] = json_data["debug"];}
        } catch(json::parse_error& err) {
            printf("[!] Error parsing config.json: %s\n", err.what());
        }

        ifs.close();
    }

    // convert current maps into string arrays
    string keymap_strings[12];
    string buttonmap_strings[12];

    for (int i = 0; i < 12; i++) {
        keymap_strings[i] = SDL_GetKeyName(keymap[i]);
        buttonmap_strings[i] = SDL_GameControllerGetStringForButton(buttonmap[i]);
    }

    new_config["music_volume"] = music_volume;
    new_config["sfx_volume"] = sfx_volume;
    new_config["mono_toggle"] = mono_toggle;
    new_config["display_fps"] = fps_toggle;
    new_config["fullscreen"] = fullscreen_toggle;
    new_config["vsync"] = vsync_toggle;
    new_config["frame_cap"] = frame_cap;
    new_config["display_grid"] = grid_toggle;
    new_config["display_hud"] = hud_toggle;
    new_config["blindfold_mode"] = blindfold_toggle;
    new_config["controller_rumble"] = rumble_toggle;
    new_config["controller_index"] = controller_index;
    new_config["key_map"] = keymap_strings;
    new_config["button_map"] = buttonmap_strings;

    printf("Saving to config.json...\n");

    std::ofstream file("config.json");
    file << new_config.dump(4);

    return;
}

void load_settings(int argc, char* argv[]) {
    printf("Loading configuration...\n");
    std::ifstream ifs("config.json");
    json json_data;

    if (ifs.fail()) {
        printf("[!] config.json not found, creating one...\n");
        save_settings();
        return;
    }

    try {
        json_data = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Error parsing config.json: %s\n", err.what());
        save_settings();
        return;
    }

    // if we're at this point, that means the JSON was parsed correctly
    if (json_data.contains("display_fps"))       {fps_toggle = json_data["display_fps"];}
    if (json_data.contains("fullscreen"))        {fullscreen_toggle = json_data["fullscreen"];}
    if (json_data.contains("vsync"))             {vsync_toggle = json_data["vsync"];}
    if (json_data.contains("frame_cap"))         {frame_cap = json_data["frame_cap"];}
    if (json_data.contains("display_grid"))      {grid_toggle = json_data["display_grid"];}
    if (json_data.contains("display_hud"))       {hud_toggle = json_data["display_hud"];}
    if (json_data.contains("blindfold_mode"))    {blindfold_toggle = json_data["blindfold_mode"];}
    if (json_data.contains("music_volume"))      {music_volume = json_data["music_volume"];}
    if (json_data.contains("sfx_volume"))        {sfx_volume = json_data["sfx_volume"];}
    if (json_data.contains("mono_toggle"))       {mono_toggle = json_data["mono_toggle"];}
    if (json_data.contains("controller_rumble")) {rumble_toggle = json_data["controller_rumble"];}
    if (json_data.contains("controller_index"))  {controller_index = json_data["controller_index"];}

    // populates the keymap
    if (json_data.contains("key_map")) {
        printf("Reading keyboard mappings...\n");
        for (int i = 0; i < 12; i++) {
            std::string key_name = json_data["key_map"][i];
            SDL_Keycode key_code = SDL_GetKeyFromName(key_name.c_str());

            if (key_code == SDLK_UNKNOWN) {printf("[!] Unrecognized keycode: %s\n", key_name.c_str()); continue;}
            keymap[i] = key_code;
        }
    }

    // populates the button map
    if (json_data.contains("button_map")) {
        printf("Reading button mappings...\n");
        for (int i = 0; i < 12; i++) {
            std::string button_name = json_data["button_map"][i];
            SDL_GameControllerButton button_code = SDL_GameControllerGetButtonFromString(button_name.c_str());

            if (button_code == SDL_CONTROLLER_BUTTON_INVALID) {printf("[!] Unrecognized button: %s\n", button_name.c_str()); continue;}
            buttonmap[i] = button_code;
        }
    }

    // applies the newly-loaded frame cap values
    set_frame_cap_ms();

    // note these parameters, they aren't exposed in the options and aren't saved in the config by default
    // however, they can still be added manually to a config if a user desires
    if (json_data.contains("game_width")) {width = json_data["game_width"];}
    if (json_data.contains("game_height")) {height = json_data["game_height"];}
    if (json_data.contains("debug")) {debug_toggle = json_data["debug"];}

    // parse any command-line arguments relevant to game settings here
    // this is done after loading; i.e. overriding whatever settings we have
    if (parse_option(argv, argv+argc, "-fullscreen") || parse_option(argv, argv+argc, "-f")) {printf("Enabling borderless fullscreen...\n"); fullscreen_toggle = true;}
    if (parse_option(argv, argv+argc, "-vsync") || parse_option(argv, argv+argc, "-v")) {printf("Enabling vertical sync...\n"); vsync_toggle = true;}

    if (parse_option(argv, argv+argc, "-true-fullscreen") || parse_option(argv, argv+argc, "-tf")) {
        printf("[!] Enabling true fullscreen. Graphical bugs may occur!\n");
        fullscreen_toggle = false;
        true_fullscreen_toggle = true;
    }

    if (parse_option(argv, argv+argc, "-debug") || parse_option(argv, argv+argc, "-d") || debug_toggle) {
        printf("[!] Enabling debug features. These are intentionally undocumented and may have odd behavior!\n");
        debug_toggle = true;
    }

    return;
}

void load_levels() {
    // usually ran just once during game startup, so speed isn't *that* important here
    // scans for level folders, then loads the level_paths vector with any valid levels

    const std::filesystem::path levels{"assets/levels"};
    unsigned int scanned_level_count = 0; // unsigned since a negative level count makes no sense
    vector<std::filesystem::path> playlist_paths;
    vector<std::filesystem::path> folder_paths;

    printf("Scanning for levels...\n");

    if (!std::filesystem::is_directory(levels)) {
        printf("[!] The levels directory (%s) couldn't be found!\n", levels.string().c_str());
        return;
    }

    // scans the levels folder for all files and folders, populating two vectors to be processed separately
    for (auto& dir_entry: std::filesystem::directory_iterator{levels}) {
        if (std::filesystem::is_directory(dir_entry.path())) {
            folder_paths.push_back(dir_entry.path());
            continue;
        }

        if (dir_entry.path().extension() == ".json") {
            playlist_paths.push_back(dir_entry.path());
            continue;
        }
    }

    // sorts the aeformentioned vectors alphabetically (not guaranteed by std::filesystem!)
    std::sort(playlist_paths.begin(), playlist_paths.end());
    std::sort(folder_paths.begin(), folder_paths.end());

    printf("Processing playlists...\n");
    for (std::filesystem::path dir_entry: playlist_paths) {
        std::ifstream ifs(dir_entry);
        json playlist_data;
        string playlist_name;

        // checks to see if the JSON is valid JSON
        try {
            playlist_data = json::parse(ifs);
        } catch(json::parse_error& err) {
            printf("[!] Error parsing playlist file: %s\n", err.what());
            continue;
        }

        playlist_name = playlist_data[0].value("name", "Untitled Playlist");
        printf("Parsing playlist: %s\n", playlist_name.c_str());

        for (unsigned int i = 0; i < playlist_data[1]["levels"].size(); i++) {
            std::filesystem::path playlist_level = levels / playlist_data[1]["levels"][i];

            if (std::filesystem::exists(playlist_level.string())) {
                // checks if the level isn't present already
                if (std::find(level_paths.begin(), level_paths.end(), playlist_level.string()) != level_paths.end()) {continue;}

                // checks if the folder contains a level.json
                bool contains_level = false;

                for (auto& file_entry: std::filesystem::directory_iterator{playlist_level}) {
                    string filename = file_entry.path().filename().string();
                    if (filename == "level.json") {contains_level = true; break;}
                }

                if (!contains_level) {continue;}

                // okay it isn't, cool, add it to the list
                level_paths.push_back(playlist_level.string());
                level_playlists.push_back(playlist_name);
                scanned_level_count++;

                printf("Added level from playlist: %s\n", playlist_level.string().c_str());
            }
        }
    }

    printf("Processing level folders...\n");
    for (std::filesystem::path dir_entry: folder_paths) {
        // checks if the level isn't present from a playlist already, prevents duplicated levels
        if (std::find(level_paths.begin(), level_paths.end(), dir_entry.string()) != level_paths.end()) {continue;}

        // checks for the existence of a level.json file within the current directory
        for (auto& subdir_entry: std::filesystem::directory_iterator{dir_entry}) {
            string filename = subdir_entry.path().filename().string();

            if (filename == "level.json") {
                string level_path = dir_entry.string();
                level_paths.push_back(level_path);
                level_playlists.push_back("");
                scanned_level_count++;

                printf("Added level: %s\n", level_path.c_str());
                break;
            }
        }
    }

    if (scanned_level_count == 0) {
        printf("[!] No levels were found!\n");
    } else if (scanned_level_count == 1) {
        printf("Found 1 level.\n");
    } else {
        printf("Found %i levels.\n", scanned_level_count);
    }

    return;
}

void load_motd() {
    // checks for the existence of motd.txt, and if found, loads a random splash line from it
    // this is ran only once during game startup and is just for fun

    std::ifstream file("assets/motd.txt");
    string dummy;
    int file_line_count = 0;
    int i = 0;

    if (std::filesystem::exists("assets/motd.txt") == false) {
        printf("motd.txt is not present, MOTD will be blank.\n");
        return;
    }

    // count the number of lines in motd.txt so we can get an upper bound
    while (std::getline(file, dummy)) {file_line_count++;}

    if (file_line_count == 0) {
        printf("motd.txt present, but contains no text...?\n");
        motd = "missingno";
        return;
    }

    int random = rand() % file_line_count;

    // reset ifstream flags so we're back at the start of MOTD.txt
    file.clear();
    file.seekg(0);

    // loop over motd.txt until we get to the randomly-selected line
    while (std::getline(file, dummy)) {
        i++;

        if (i == random) {
            break;
        }
    }

    motd = dummy;
    printf("Got MOTD: %s\n", dummy.c_str());
    return;
}

void load_common_sounds() {
    // loads sounds used pretty much everywhere
    // these only get loaded once on startup

    printf("Loading common sound effects...\n");
    snd_menu_move = Mix_LoadWAV("assets/sound/move.ogg");
    if(snd_menu_move == NULL) {
        printf("[!] move.ogg: %s\n", Mix_GetError());
    }

    snd_menu_confirm = Mix_LoadWAV("assets/sound/confirm.ogg");
    if(snd_menu_confirm == NULL) {
        printf("[!] confirm.ogg: %s\n", Mix_GetError());
    }

    snd_menu_back = Mix_LoadWAV("assets/sound/back.ogg");
    if(snd_menu_back == NULL) {
        printf("[!] back.ogg: %s\n", Mix_GetError());
    }

    snd_mono_test = Mix_LoadWAV("assets/sound/mono_test.ogg");
    if(snd_mono_test == NULL) {
        printf("[!] mono_test.ogg: %s\n", Mix_GetError());
    }

    snd_metronome_small = Mix_LoadWAV("assets/sound/metronome_small.ogg");
    if(snd_metronome_small == NULL) {
        printf("[!] metronome_small.ogg: %s\n", Mix_GetError());
    }

    snd_metronome_big = Mix_LoadWAV("assets/sound/metronome_big.ogg");
    if(snd_metronome_big == NULL) {
        printf("[!] metronome_big.ogg: %s\n", Mix_GetError());
    }

    return;
}

void init_controller() {
    // checks for and initializes gamecontrollerdb.txt
    // see SDL2's documentation for more info

    if (SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") != -1) {
        printf("gamecontrollerdb.txt mappings loaded.\n");
    }

    // closes current controller
    SDL_GameControllerClose(controller);
    controller = NULL;

    controller = SDL_GameControllerOpen(controller_index);

    if (controller == NULL) {
        printf("[!] Error initializing controller: %s\n", SDL_GetError());
    } else {
        printf("Controller initialized: %s\n", SDL_GameControllerName(controller));
    }

    return;
}

int get_controller_count() {
    if (SDL_NumJoysticks() <= 0) return 0;
    return SDL_NumJoysticks() - 1;
}

void reset_keyboard_binds() {
    SDL_KeyCode default_map[12] = {
        SDLK_UP,
        SDLK_DOWN,
        SDLK_LEFT,
        SDLK_RIGHT,
        SDLK_z,
        SDLK_x,
        SDLK_c,
        SDLK_v,
        SDLK_a,
        SDLK_s,
        SDLK_RETURN,
        SDLK_BACKSPACE
    };

    printf("Resetting keyboard binds...\n");
    for (int i = 0; i < 12; i++) {
        keymap[i] = default_map[i];
    }

    return;
}

void reset_controller_binds() {
    SDL_GameControllerButton default_map[12] = {
        SDL_CONTROLLER_BUTTON_DPAD_UP,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
        SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_BACK
    };

    printf("Resetting controller binds...\n");
    for (int i = 0; i < 12; i++) {
        buttonmap[i] = default_map[i];
    }

    return;
}

void rumble_controller(int ms = 120) {
    // wrapper that rumbles the controller
    // ms: how long in milliseconds to rumble the controller (optional!)

    if (rumble_toggle) {SDL_GameControllerRumble(controller, 0xFFFF, 0xFFFF, ms);}
    return;
}

void set_music_volume() {
    // wrapper that scales volume (0-100) to mix_volume's range (0-128)
    Mix_VolumeMusic(music_volume * 1.28);
    return;
}

void set_sfx_volume() {
    // wrapper that scales volume (0-100) to mix_volume's range (0-128)
    Mix_Volume(-1, sfx_volume * 1.28);
    return;
}

void downmix_to_mono(int chan, void *stream, int len, void *udata) {
    // post-process effect that mixes the audio to mono
    // implementation by @icculus of the SDL forums:
    // https://discourse.libsdl.org/t/switch-between-stereo-and-mono-sound/12013/2
    Sint16 *ptr = (Sint16*) stream;

    for (int i = 0; i < len; i += sizeof(Sint16) * 2, ptr += 2) {
        ptr[0] = ptr[1] = (ptr[0] + ptr[1]) / 2;
    }

    return;
}

void set_channel_mix() {
    // wrapper that toggles mono-downmixing
    if (mono_toggle) {
        printf("Audio outputting in mono.\n");
        Mix_RegisterEffect(MIX_CHANNEL_POST, downmix_to_mono, NULL, NULL);
    } else {
        printf("Audio outputting in stereo.\n");
        Mix_UnregisterEffect(MIX_CHANNEL_POST, downmix_to_mono);
    }

    return;
}

void play_channel_test() {
    // called from options.cpp, plays the mono test sound when toggling channel mix
    Mix_PlayChannel(-1, snd_mono_test, 0);
    return;
}

void play_dialog_blip() {
    // called from tutorial.cpp
    Mix_PlayChannel(-1, snd_metronome_big, 0);
    return;
}

void play_dialog_advance() {
    // called from tutorial.cpp
    Mix_PlayChannel(-1, snd_menu_confirm, 0);
    return;
}

void set_vsync_renderer() {
    // sets VSYNC flag in SDL2 and re-creates the renderer
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, to_string(vsync_toggle).c_str());

    SDL_DestroyRenderer(renderer);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    if (renderer == NULL) {
        printf("[!] Error re-creating renderer: %s\n", SDL_GetError());
    }

    // need to reload font texture as well, since destroying the renderer also destroys textures
    load_font();
    return;
}

string get_timestamp() {
    time_t rawtime;
    struct tm *timeinfo;
    char out[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(out, sizeof(out), "%Y-%m-%d_%H-%M-%S", timeinfo);
    return out;
}

void take_screenshot() {
    string filename = get_timestamp().append(".png");
    printf("Saving screenshot: %s\n", filename.c_str());

    // take the screenshot and save it as PNG
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
    IMG_SavePNG(surface, filename.c_str());
    SDL_FreeSurface(surface);
    return;
}

void export_shapes() {
    string filename = get_timestamp().append(".json");
    printf("Exporting JSON as: %s\n", filename.c_str());

    // converts previous_shapes into JSON data
    nlohmann::ordered_json exported_data;

    // adds a header, so this data can be used directly as level data!
    exported_data[0]["name"] = filename;
    exported_data[0]["bg_color"] = 15;

    for (int i = 0; i < previous_shapes.size(); i++) {
        exported_data[i+1]["shape"] = previous_shapes[i].type;
        exported_data[i+1]["x"] = previous_shapes[i].x;
        exported_data[i+1]["y"] = previous_shapes[i].y;
        exported_data[i+1]["scale"] = previous_shapes[i].scale;
        exported_data[i+1]["color"] = previous_shapes[i].color;
    }

    std::ofstream file(filename);
    file << exported_data.dump(4);
    return;
}

controller_buttons keyboard_to_abstract_button(SDL_Keycode input, bool in_game) {
    // converts keyboard keys to the abstract controller

    // we're using if statements here because "switch" doesn't
    // accept array members as case arguments (unfortuantely)
    if (input == keymap[0])         return UP;
    if (input == keymap[1])         return DOWN;
    if (input == keymap[2])         return LEFT;
    if (input == keymap[3])         return RIGHT;
    if (input == keymap[8])         return LB;
    if (input == keymap[9])         return RB;
    if (input == keymap[10])        return START;
    if (input == keymap[11])        return SELECT;

    // the bool is there to switch keys around between menus/gameplay
    // this is done to make the menu more intuitive to control on KB
    if (in_game) {
        if (input == keymap[4])     return CIRCLE;
        if (input == keymap[5])     return SQUARE;
        if (input == keymap[6])     return TRIANGLE;
        if (input == keymap[7])     return CROSS;
    } else {
        if (input == keymap[4])     return CROSS;
        if (input == keymap[5])     return CIRCLE;
        if (input == keymap[6])     return SQUARE;
        if (input == keymap[7])     return TRIANGLE;
    }

    return NONE;
}

controller_buttons gamepad_to_abstract_button(int input) {
    // similar to above, but for gamepads
    // there's no bool switch here since it isnt necessary

    if (input == buttonmap[0])     return UP;
    if (input == buttonmap[1])     return DOWN;
    if (input == buttonmap[2])     return LEFT;
    if (input == buttonmap[3])     return RIGHT;
    if (input == buttonmap[4])     return CIRCLE;
    if (input == buttonmap[5])     return SQUARE;
    if (input == buttonmap[6])     return TRIANGLE;
    if (input == buttonmap[7])     return CROSS;
    if (input == buttonmap[8])     return LB;
    if (input == buttonmap[9])     return RB;
    if (input == buttonmap[10])    return START;
    if (input == buttonmap[11])    return SELECT;

    return NONE;
}

bool init(int argc, char *argv[]) {
    // initializes SDL, loads common files like default SFX/font, and loads config
    // returns true if all goes well, false if something can't initialize

    print_header();
    printf("Initializing...\n");

    // loads config and sets SDL hints
    load_settings(argc, argv);

    // sets VSYNC render hint
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, to_string(vsync_toggle).c_str());

    // sets preferred backend to openGL
    // this fixes, among other things (likely), the starfield BGFX
    // see https://github.com/open-manifold/Open-Manifold/issues/22
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    // initialize SDL stuff (video, audio, inputs, events, etc.)
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] Error initializing SDL: %s\n", SDL_GetError());
        return false;
    }

    // create window
    printf("Creating window with resolution %i x %i...\n", width, height);
    window = SDL_CreateWindow("Open Manifold", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);

    if (window == NULL) {
        printf("[!] Error creating window: %s\n", SDL_GetError());
        return false;
    }

    // enables fullscreen(s) if applicable
    set_fullscreen();

    if (true_fullscreen_toggle) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    // create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    if (renderer == NULL) {
        printf("[!] Error creating renderer: %s\n", SDL_GetError());
        return false;
    }

    // random number initialization
    srand(time(NULL));

    // loads the font and draws loading screen
    load_font();
    draw_loading();

    // starts up audio and sets volumes
    printf("Creating audio mixer...\n");
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("[!] Error creating audio mixer: %s\n", Mix_GetError());
        return false;
    }

    set_music_volume();
    set_sfx_volume();
    set_channel_mix();
    load_common_sounds();

    // initializes controller, if one is detected
    printf("Checking for controllers...\n");
    if (SDL_NumJoysticks() >= 1) {
        printf("Controller detected, initializing...\n");
        init_controller();
    } else {
        printf("No controllers were detected.\n");
    }

    printf("Initialized successfully!\n");
    return true;
}

void kill() {
    printf("Quitting game...\n");
    printf("End of log.");
    fclose(stdout);
    Mix_CloseAudio();
    SDL_GameControllerClose(controller);
    controller = NULL;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int get_life() {
    return life;
}

int get_score() {
    return score;
}

int get_combo() {
    return combo;
}

void modify_life(int mod) {
    // adds or subtracts life value; wrapped up to ensure it's capped consistently
    life += mod;

    if (life < 0) {life = 0;}
    if (life > 100) {life = 100;}
    return;
}

void reset_score() {
    score = 0;
    combo = 0;
    set_combo_timer(0);
    return;
}

void reset_life() {
    life = 100;
    return;
}

void reset_score_and_life() {
    // wrapper for two other functions
    reset_score();
    reset_life();
    return;
}

string get_motd() {
    return motd;
}

string get_level_json_path() {
    string path = level_paths[level_index] + "/level.json";
    return path;
}

string get_background_tile_path() {
    string path = level_paths[level_index] + "/tile.png";
    return path;
}

string get_character_tile_path() {
    string path = level_paths[level_index] + "/character.png";
    return path;
}

string get_level_name() {
    if (json_file == NULL) {return "Untitled";}

    return json_file[0].value("name", "Untitled");
}

string get_level_playlist_name() {
    string name = level_playlists[level_index];
    if (name == "") {return "None";}

    return name;
}

string get_genre() {
    if (json_file == NULL) {return "Unknown";}

    return json_file[0].value("genre", "Unknown");
}

string get_level_author() {
    if (json_file == NULL) {return "Anonymous";}

    return json_file[0].value("level_author", "Anonymous");
}

string get_song_author() {
    if (json_file == NULL) {return "Anonymous";}

    return json_file[0].value("song_author", "Anonymous");
}

int get_hiscore() {
    return metadata.hiscore;
}

int get_play_count() {
    return metadata.play_count;
}

bool get_cleared() {
    return metadata.cleared;
}

string get_cpu_sequence() {
    return cpu_sequence;
}

string get_player_sequence() {
    return player_sequence;
}

int get_level_bpm() {
    if (json_file == NULL) {return 120;}

    return json_file[0].value("bpm", 120);
}

int get_level_time_signature(bool top_or_bottom) {
    if (top_or_bottom) {
        return json_file[0].value("time_signature_top", 4);
    } else {
        return json_file[0].value("time_signature_bottom", 4);
    }
}

int get_level_measure_length() {
    int top = get_level_time_signature(true);
    int bot = get_level_time_signature(false);
    return top * bot;
}

int get_level_intro_delay() {
    int sequence_length = get_level_measure_length() * 2;

    return json_file[0].value("offset", sequence_length);
}

int get_bg_color() {
    int value = 15;

    // "bgColor" is a legacy/deprecated property, and should not be used anymore
    if (json_file[0].contains("bgColor")) {
        value = json_file[0].value("bgColor", 15);
    }

    // we give bg_color (the newer one) higher priority
    if (json_file[0].contains("bg_color")) {
        value = json_file[0].value("bg_color", 15);
    }

    return value;
}

int get_song_step(int index, int placeholder) {
    // gets value of song_step for a given shape index in a file

    if (index > json_file.size()) {return placeholder;}

    if (json_file[index].contains("song_step")) {
        return json_file[index].value("song_step", placeholder);
    }

    return placeholder;
}

string get_level_background_effect_string() {
    return json_file[0].value("background_effect", "none");
}

bool get_debug() {
    return debug_toggle;
}

string get_current_mapping() {
    // used in the options menu when rebinding
    // returns a string of the current binding for a given index
    unsigned int index = get_rebind_index();

    if (check_rebind_keys()) {
        return SDL_GetKeyName(keymap[index]);
    }

    if (check_rebind_controller()) {
        return SDL_GameControllerGetStringForButton(buttonmap[index]);
    }

    return "?";

}

string get_input_name() {
    // used in the options menu when rebinding
    // returns a string of the abstract controller that everything is mapped onto; named after the PS1 pad
    // see controller_buttons above
    unsigned int index = get_rebind_index();

    switch (index) {
        case 0: return "Up";
        case 1: return "Down";
        case 2: return "Left";
        case 3: return "Right";
        case 4: return "Circle";
        case 5: return "Square";
        case 6: return "Triangle";
        case 7: return "Cross";
        case 8: return "L1";
        case 9: return "R1";
        case 10: return "Start";
        case 11: return "Back";
        default: return "?";
    }
}

int calculate_score() {
    // calculates a score to give the player by comparing sequence strings
    // ----------------------------------------------------------
    // SCORE TABLE IS: 100 for any ops that match CPU (except blanks), 50 for new ops, 25 for new xplodes
    int score = 0;
    string cpu_sequence = get_cpu_sequence();
    string player_sequence = get_player_sequence();

    if (get_debug()) {
        printf("CPU: %s\nPLY: %s\n", cpu_sequence.c_str(), player_sequence.c_str());
    }

    for (int i = 0; i < player_sequence.length(); i++) {
        char cpu_op = cpu_sequence[i];
        char player_op = player_sequence[i];

        if (cpu_op == '.') {
            if (player_op == '.') {continue;}
            if (player_op == 'X') {score += 25; continue;}
            score += 50;
        }

        // if we're at this point in the loop, then we know that:
        // score is 50, cpu_op isn't blank, player_op also isn't blank or an xplode

        if (cpu_op == player_op) {score += 50; continue;}
    }

    return score;
}

void load_metadata() {
    int score = 0;
    int play_count = 0;
    bool cleared = false;
    string current_level = get_level_name();
    std::ifstream ifs("hiscores.json");

    if (ifs.good()) {
        try {
            json json_data = json::parse(ifs);

            if (json_data.contains(current_level)) {
                score = json_data[current_level].value("score", 0);
                play_count = json_data[current_level].value("play_count", 0);
                cleared = json_data[current_level].value("cleared", false);
            }
        } catch(json::parse_error& err) {
            printf("[!] Error parsing hiscores.json: %s\n", err.what());
        }

        ifs.close();
    }

    metadata.hiscore = score;
    metadata.play_count = play_count;
    metadata.cleared = cleared;
    return;
}

void save_score() {
    // saves the current score value to the currently selected level's entry in hiscores.json
    int current_hiscore = 0;
    int new_score = get_score();
    string current_level = get_level_name();

    json json_data;
    std::ifstream ifs("hiscores.json");

    if (ifs.good()) {
        try {
            json_data = json::parse(ifs);
        } catch(json::parse_error& err) {
            printf("[!] Error parsing hiscores.json: %s\n", err.what());
        }

        ifs.close();
    }

    // check if level has a saved hiscore already
    if (json_data.contains(current_level)) {
        current_hiscore = json_data[current_level].value("score", 0);
    }

    // compare old score to new score, save it only if it's higher!
    if (new_score > current_hiscore) {
        printf("Saving new hi-score for %s...\n", current_level.c_str());
        json_data[current_level]["score"] = new_score;
        std::ofstream file("hiscores.json");
        file << json_data.dump(4);
    }

    return;
}

void save_play_count() {
    // saves the current play count to the currently selected level's entry in hiscores.json
    // or more accurately it just increments it by 1
    unsigned int play_count = metadata.play_count;
    string current_level = get_level_name();

    json json_data;
    std::ifstream ifs("hiscores.json");

    if (ifs.good()) {
        try {
            json_data = json::parse(ifs);
        } catch(json::parse_error& err) {
            printf("[!] Error parsing hiscores.json: %s\n", err.what());
        }

        ifs.close();
    }

    printf("Saving play count for %s...\n", current_level.c_str());
    json_data[current_level]["play_count"] = play_count + 1;
    std::ofstream file("hiscores.json");
    file << json_data.dump(4);

    return;
}

void save_cleared() {
    // saves a cleared flag to the currently selected level's entry in hiscores.json
    bool cleared = false;
    string current_level = get_level_name();

    json json_data;
    std::ifstream ifs("hiscores.json");

    if (ifs.good()) {
        try {
            json_data = json::parse(ifs);
        } catch(json::parse_error& err) {
            printf("[!] Error parsing hiscores.json: %s\n", err.what());
        }

        ifs.close();
    }

    // check if level has been cleared already
    if (json_data.contains(current_level)) {
        cleared = json_data[current_level].value("cleared", false);
    }

    // save cleared flag if it hasn't been cleared before
    if (metadata.cleared && !cleared) {
        printf("Saving level clear flag for %s...\n", current_level.c_str());
        json_data[current_level]["cleared"] = true;
        std::ofstream file("hiscores.json");
        file << json_data.dump(4);
    }
}

void save_metadata() {
    // wrapper for the above three functions
    save_score();
    save_play_count();
    save_cleared();
    return;
}

bool check_json_validity() {
    // returns false if the level json object is NULL
    // used in the level select in graphics.cpp

    return json_file != NULL;
}

void load_tile_frame_file() {
    string file = level_paths[level_index] + "/tile.json";
    std::ifstream ifs(file);
    json parsed_json;

    printf("Loading tile frames file: %s\n", file.c_str());

    // checks to make sure the file exists
    if (std::filesystem::exists(file) == false) {
        printf("Tile frames file does not exist, skipping...\n");
        fallback_tile_frames();
        return;
    }

    // checks to see if the JSON is valid JSON
    try {
        parsed_json = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Error parsing tile frames file: %s\n", err.what());
        fallback_tile_frames();
        return;
    }

    parse_tile_frames(parsed_json);
    return;
}

void load_character_file() {
    string file = level_paths[level_index] + "/character.json";
    std::ifstream ifs(file);
    json parsed_json;

    printf("Loading character file: %s\n", file.c_str());

    // checks to make sure the file exists
    if (std::filesystem::exists(file) == false) {
        printf("Character file does not exist, skipping...\n");
        return;
    }

    // checks to see if the JSON is valid JSON
    try {
        parsed_json = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Error parsing character file: %s\n", err.what());
        return;
    }

    parse_character_file(parsed_json);
    load_character_tileset();
    return;
}

void load_default_music(string name) {
    Mix_HaltMusic();
    Mix_FreeMusic(music);

    string filename = "assets/music/" + name + ".ogg";
    printf("Loading music: %s\n", filename.c_str());
    music = Mix_LoadMUS(filename.c_str());

    if(music == NULL) {
        printf("%s\n", Mix_GetError());
    } else {
        Mix_PlayMusic(music, -1);
    }
    return;
}

void load_stage_music() {
    Mix_HaltMusic();
    Mix_FreeMusic(music);

    string filename = level_paths[level_index] + "/song.ogg";
    printf("Loading music: %s\n", filename.c_str());
    music = Mix_LoadMUS(filename.c_str());

    if(music == NULL) {
        printf("%s\n", Mix_GetError());
    }
    return;
}

Mix_Chunk* load_default_sound(string file_name) {
    // loads default sounds, in case custom sounds dont exist or otherwise fail to load
    // also used for the sandbox mode

    string path = "assets/sound/" + file_name + ".ogg";
    const char* path_cstr = path.c_str();

    Mix_Chunk* sound = Mix_LoadWAV(path_cstr);

    if (sound == NULL) {
        printf("%s\n", Mix_GetError());
    } else {
        printf("Loaded default sound: %s\n", path_cstr);
    }
    return sound;
}

Mix_Chunk* load_stage_sound(string file_name) {
    // returns a loaded sound effect for the currently-loaded level
    // this gets used multiple times every time a level is loaded

    string path = level_paths[level_index] + "/" + file_name + ".ogg";
    const char* path_cstr = path.c_str();
    Mix_Chunk* sound;

    sound = Mix_LoadWAV(path_cstr);

    // load fallback if sound doesn't/can't exist
    if(sound == NULL) {
        sound = load_default_sound(file_name);
    } else {
        printf("Loaded sound: %s\n", path_cstr);
    }

    return sound;
}

void unload_sounds() {
    // frees every sound file used in levels

    Mix_FreeChunk(snd_up);
    Mix_FreeChunk(snd_down);
    Mix_FreeChunk(snd_left);
    Mix_FreeChunk(snd_right);
    Mix_FreeChunk(snd_circle);
    Mix_FreeChunk(snd_square);
    Mix_FreeChunk(snd_triangle);
    Mix_FreeChunk(snd_xplode);
    Mix_FreeChunk(snd_scale_up);
    Mix_FreeChunk(snd_scale_down);
    Mix_FreeChunk(snd_success);
    Mix_FreeChunk(snd_combo);

    return;
}

void load_stage_sound_collection() {
    // wrapper that loads every sound a level can use aside from music
    // uses the load_stage_sound function below

    unload_sounds();

    snd_up          = load_stage_sound("up");
    snd_down        = load_stage_sound("down");
    snd_left        = load_stage_sound("left");
    snd_right       = load_stage_sound("right");
    snd_circle      = load_stage_sound("circle");
    snd_square      = load_stage_sound("square");
    snd_triangle    = load_stage_sound("triangle");
    snd_xplode      = load_stage_sound("xplode");
    snd_scale_up    = load_stage_sound("scale_up");
    snd_scale_down  = load_stage_sound("scale_down");
    snd_success     = load_stage_sound("success");
    snd_combo       = load_stage_sound("combo");

    return;
}

void load_default_sound_collection() {
    // similar to above but specifically for default sounds
    // this is used by the sandbox mode

    unload_sounds();

    snd_up          = load_default_sound("up");
    snd_down        = load_default_sound("down");
    snd_left        = load_default_sound("left");
    snd_right       = load_default_sound("right");
    snd_circle      = load_default_sound("circle");
    snd_square      = load_default_sound("square");
    snd_triangle    = load_default_sound("triangle");
    snd_xplode      = load_default_sound("xplode");
    snd_scale_up    = load_default_sound("scale_up");
    snd_scale_down  = load_default_sound("scale_down");
    snd_success     = load_default_sound("success");
    snd_combo       = load_default_sound("combo");

    return;
}

/**
 * returns true if either fade value is anything other than 0
 * in other words, this is true as long as any fade is happening
 */
bool check_fade_activity() {
    return fade_in != 0 || fade_out != 0;
}

/**
 * similar to check_fade_activity, but allows fade_in to be active
 */
bool check_fade_in_activity() {
    return (fade_in <= 1) && (fade_out == 0);
}

void fade_reset() {
    fade_out = 0;
    fade_in = 255;
    return;
}

void reset_shapes() {
    // resets shapes to default values
    result_shape.type = 0;
    result_shape.x = 7;
    result_shape.y = 7;
    result_shape.scale = 1;
    result_shape.color = 0;

    active_shape.type = -1; // this makes the shape render invisible by default
    active_shape.x = 7;
    active_shape.y = 7;
    active_shape.scale = 1;
    active_shape.color = 0;

    return;
}

void reset_sequences() {
    // resets sequences to blank strings
    int measure_length = get_level_measure_length();

    cpu_sequence.clear();
    player_sequence.clear();
    cpu_sequence.insert(cpu_sequence.begin(), measure_length, '.');
    player_sequence.insert(player_sequence.begin(), measure_length, '.');

    return;
}

int check_beat_timing_window(unsigned int current_time) {
    // returns 0 if we're not within the beat, 1 on the "left side" (beat end), 2 on the right side (beat start)
    // the window should be within ~120 ms, or 60ms on each "side" of a beat (roughly equivalent to a "Good" in DDR)

    int current_beat_length = (current_time - beat_start_time);
    int time_to_next_beat = length - current_beat_length;
    int measure_length = get_level_measure_length();
    int start_offset = get_level_intro_delay();
    bool valid_beat_window_start = false;
    bool valid_beat_window_end = false;

    // this statement excludes all of the intro
    if (song_start_time + intro_beat_length >= current_time) {return 0;}

    // and this excludes the end of the song
    if (song_over) {return 0;}

    // this excludes beats where the CPU is in control, as well as parts of beats that shouldnt be 'allowed'
    // this means that there's not measure_length of valid beats, but rather measure_length of windows
    if ((beat_count - start_offset)%(measure_length*2) >= measure_length) {valid_beat_window_start = true;}
    if (((beat_count - start_offset) - 1)%(measure_length*2) >= measure_length) {valid_beat_window_end = true;}

    // finally, checks the timing window
    if (valid_beat_window_end && current_beat_length <= 60) {return 1;}
    if (valid_beat_window_start && time_to_next_beat <= 60) {return 2;}

    return 0;
}

bool compare_shapes(shape shape_1, shape shape_2) {
    // returns true if the player and CPU shapes match properties

    if (shape_1.x != shape_2.x) return false;
    if (shape_1.y != shape_2.y) return false;
    if (shape_1.type != shape_2.type) return false;
    if (shape_1.scale != shape_2.scale) return false;

    return true;
}

shape modify_current_shape(char opcode, shape current_shape, bool is_player = false, bool play_sound = true) {
    // modifies the shape passed into it using an "opcode"
    // ----------------------------------------------------------
    // opcode: single letter that represents an action (corresponding to keyboard controls)
    // shape: the shape parameters to modify
    // is_player: whether this modification is from the CPU or the player

    shape modified_shape = current_shape;

    // check to see if the shape is visible
    if (modified_shape.type == -1) {
        // check to see if the opcode isnt one that makes said shape visible
        if (opcode != 'Z' && opcode != 'X' && opcode != 'C') return modified_shape;
    }

    if (is_player) {
        set_character_status(opcode);
    }

    switch (opcode) {
        // circle
        case 'Z':
            if (play_sound) {Mix_PlayChannel(-1, snd_circle, 0);}
            modified_shape.type = 0;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // square
        case 'X':
            if (play_sound) {Mix_PlayChannel(-1, snd_square, 0);}
            modified_shape.type = 1;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // triangle
        case 'C':
            if (play_sound) {Mix_PlayChannel(-1, snd_triangle, 0);}
            modified_shape.type = 2;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // x-plode (essentially a NOP)
        case 'V':
            if (play_sound) {Mix_PlayChannel(-1, snd_xplode, 0);}
            break;

        // shrink
        case 'A':
            if (play_sound) {Mix_PlayChannel(-1, snd_scale_down, 0);}
            modified_shape.scale = fmax(1, modified_shape.scale - 1);
            break;

        // grow
        case 'S':
            if (play_sound) {Mix_PlayChannel(-1, snd_scale_up, 0);}
            modified_shape.scale = fmin(8, modified_shape.scale + 1);
            break;

        // up
        case 'U':
            if (play_sound) {Mix_PlayChannel(-1, snd_up, 0);}
            modified_shape.y = fmax(modified_shape.y - 1, 0);
            break;

        // down
        case 'D':
            if (play_sound) {Mix_PlayChannel(-1, snd_down, 0);}
            modified_shape.y = fmin(modified_shape.y + 1, 14);
            break;

        // left
        case 'L':
            if (play_sound) {Mix_PlayChannel(-1, snd_left, 0);}
            modified_shape.x = fmax(modified_shape.x - 1, 0);
            break;

        // right
        case 'R':
            if (play_sound) {Mix_PlayChannel(-1, snd_right, 0);}
            modified_shape.x = fmin(modified_shape.x + 1, 14);
            break;

        // no-op
        case '.':
            break;

        default: break;
    }

    return modified_shape;
}

void morph_shapes() {
    for (int i = 0; i < previous_shapes.size(); i++) {
        int new_type = (previous_shapes[i].type + 1) % 3;
        previous_shapes[i].type = new_type;
    }
    return;
}

void morph_colors() {
    for (int i = 0; i < previous_shapes.size(); i++) {
        // skip any shapes with the erase color
        if (previous_shapes[i].color == 16) {continue;}

        int new_color = (previous_shapes[i].color + 1) % 16;
        previous_shapes[i].color = new_color;
    }
    return;
}

bool check_available_sequence(int beat_side) {
    // returns true if the current sequence op is blank, false if it isn't
    // this ensures that both a sequence can't be overwritten and that
    // only ONE input is registered, preventing "diagonals" for example
    // ----------------------------------------------------------
    // beat_side: returned from check_beat_timing_window(); 1 = beat end, 2 = beat start

    int measure_length = get_level_measure_length();
    int start_offset = get_level_intro_delay();
    int current_beat_count = (beat_count - start_offset) % measure_length;

    int index = 0;
    string sequence = get_player_sequence();

    if (beat_side == 0) {return false;}
    if (beat_side == 1) {index = current_beat_count - 1;}
    if (beat_side == 2) {index = current_beat_count;}

    if (index <= 0) {index = 0;}
    if (index >= measure_length) {index = measure_length;}

    if (sequence[index] == '.') {return true;}

    return false;
}

string modify_sequence(char opcode, int beat_side) {
    // records the player's moves to the player sequence
    // ----------------------------------------------------------
    // opcode: single letter that represents an action (see modify_current_shape())
    // beat_side: returned from check_beat_timing_window(); 1 = beat end, 2 = beat start

    int measure_length = get_level_measure_length();
    int start_offset = get_level_intro_delay();
    int current_beat_count = (beat_count - start_offset) % measure_length;

    int index = 0;
    string sequence = get_player_sequence();

    if (beat_side == 0) {return sequence;}
    if (beat_side == 1) {index = current_beat_count - 1;}
    if (beat_side == 2) {index = current_beat_count;}

    if (index <= 0) {index = 0;}
    if (index >= measure_length) {index = measure_length;}

    sequence[index] = opcode;

    return sequence;
}

bool check_sequence_validity(string sequence, shape shape_result) {
    // checks to see if a given sequence actually becomes the shape it corresponds to
    // used in parse_level_file() below

    // create blank shape to run sequence on
    shape shape_test = {
        -1,
        7,
        7,
        1,
        0
    };

    // run sequence on shape
    for (int i = 0; i < sequence.length(); i++) {
        char op = sequence[i];
        shape_test = modify_current_shape(op, shape_test, false, false);
    }

    if (get_debug()) {printf("shape_test: %i %i %i %i \nexpected: %i %i %i %i \n", shape_test.type, shape_test.x, shape_test.y, shape_test.scale, shape_result.type, shape_result.x, shape_result.y, shape_result.scale);}
    return compare_shapes(shape_result, shape_test);
}

json parse_level_file(string file) {
    // loads a JSON file and parses it, filling in blanks if needed; called every time a new level is hovered over via level select
    // ----------------------------------------------------------
    // file: a path to a file, usually supplied by get_level_json_path() ("assets/levels/foobar/level.json")

    if (get_debug()) {printf("Parsing level file: %s\n", file.c_str());}

    std::ifstream ifs(file);
    json parsed_json;

    // checks to make sure the file exists
    if (std::filesystem::exists(file) == false) {
        printf("[!] Couldn't parse level file: Does not exist.\n");
        return NULL;
    }

    // checks to see if the JSON is valid JSON
    try {
        parsed_json = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Error parsing level file: %s\n", err.what());
        return NULL;
    }

    // sets the current JSON to json_file, done to make get_level_bpm etc. function
    // (this is a janky workaround; TODO: rewrite the get_level_ functions so this isn't necessary anymore)
    json_file = parsed_json;
    bpm = get_level_bpm();

    // checks to see if a color_table exists, and if it does, try to load it
    reset_color_table();

    if (parsed_json[0].contains("color_table")) {
        if (get_debug()) {printf("Parsing color table...\n");}

        for (int i = 0; i < 16; i++) {
            if (parsed_json[0]["color_table"][i] == NULL) {continue;}
            if (parsed_json[0]["color_table"][i].is_string() == false) {continue;}

            set_color_table(i, parsed_json[0]["color_table"][i]);
        }
    }

    // this entire block generates sequences if they aren't present
    // note that we generate these even if they're already present in order to ensure a level is actually beatable
    // it also uses this opportunity to populate previous_shapes for the level select menu

    int max_sequence_length = get_level_measure_length();
    previous_shapes.clear();

    for (int i = 1; i < parsed_json.size(); i++) {
        int shape_type = parsed_json[i].value("shape", 0);
        int x = parsed_json[i].value("x", 7);
        int y = parsed_json[i].value("y", 7);
        int scale = parsed_json[i].value("scale", 1);

        bool sequence_exists = parsed_json[i].contains("sequence");
        string gen_sequence;

        // populates the previous_shapes struct
        // used for displaying the full face in the level select
        shape s = {
            shape_type,
            x,
            y,
            scale,
            parsed_json[i].value("color", 0)
        };

        previous_shapes.push_back(s);

        if (parsed_json[i].contains("auto_shapes") && parsed_json[i]["auto_shapes"].is_array()) {
            for (int j = 0; j < parsed_json[i]["auto_shapes"].size(); j++) {
                s = {
                    parsed_json[i]["auto_shapes"][j].value("shape", 0),
                    parsed_json[i]["auto_shapes"][j].value("x", 7),
                    parsed_json[i]["auto_shapes"][j].value("y", 7),
                    parsed_json[i]["auto_shapes"][j].value("scale", 1),
                    parsed_json[i]["auto_shapes"][j].value("color", 0)
                };

                previous_shapes.push_back(s);
            }
        }

        // sets the first action to changing shape
        switch (shape_type) {
            case 0: gen_sequence += "Z"; break;
            case 1: gen_sequence += "X"; break;
            case 2: gen_sequence += "C"; break;
            default: gen_sequence += "Z"; break;
        }

        // scales up the shape to the needed size
        if (scale > 1) {
            for (int j = 1; j < scale; j++) {gen_sequence += "S";}
        }

        // moves across the X axis
        if (x > 7) {
            for (int j = 7; j < x; j++) {gen_sequence += "R";}
        }

        if (x < 7) {
            for (int j = 7; j > x; j--) {gen_sequence += "L";}
        }

        // moves across the Y axis
        if (y > 7) {
            for (int j = 7; j < y; j++) {gen_sequence += "D";}
        }

        if (y < 7) {
            for (int j = 7; j > y; j--) {gen_sequence += "U";}
        }

        // checks to see if the gen_sequence can fit within the allotted number of beats
        if (gen_sequence.length() > max_sequence_length) {
            printf("[!] Generated sequence #%i is longer than max number of beats! Level is not winnable.\n", i);
        }

        // pads the sequence with NOPs
        if (gen_sequence.length() < max_sequence_length) {
            gen_sequence.insert(gen_sequence.end(), max_sequence_length - gen_sequence.length(), '.');
        }

        if (get_debug()) {printf("gen_seq: %s\n", gen_sequence.c_str());}

        // check if a sequence is defined for this shape, if not, use the generated one
        // checking this AFTER making a sequence is inefficient, but also makes it much
        // more likely to catch impossible levels and is negligible on performance
        if (sequence_exists) {
            string cur_sequence = parsed_json[i].value("sequence", ".");
            if (get_debug()) {printf("cur_seq: %s\n", cur_sequence.c_str());}
            int cur_seq_length = cur_sequence.length(); // fixes GCC warning

            // similar to the above checks for generated sequences
            if (cur_seq_length > max_sequence_length) {
                printf("Level sequence #%i is too long (must be %i, is %i)! Truncating...\n", i, max_sequence_length, cur_seq_length);
                parsed_json[i]["sequence"] = cur_sequence.substr(0, max_sequence_length);
            }

            if (cur_seq_length < max_sequence_length) {
                printf("Level sequence #%i is too short (must be %i, is %i)! Padding...\n", i, max_sequence_length, cur_seq_length);
                parsed_json[i]["sequence"] = cur_sequence.insert(cur_sequence.end(), max_sequence_length - cur_seq_length, '.');
            }

            // we create a new shape here since s has been used for auto_shapes earlier
            if (!check_sequence_validity(cur_sequence, {shape_type, x, y, scale, 0})) {
                printf("[!] Sequence #%i does not match expected shape, using fallback...\n", i);
                parsed_json[i]["sequence"] = gen_sequence;
            }

            continue;
        } else {
            parsed_json[i]["sequence"] = gen_sequence;
        }
    }

    return parsed_json;
}

bool loop(json json_file, int start_offset, int time_signature_top, int time_signature_bottom, int song_start_time, int frame_time) {
    // main gameplay loop, runs every frame during GAME
    int current_ticks = SDL_GetTicks();
    int measure_length = time_signature_top * time_signature_bottom;
    int shape_count = (beat_count - start_offset)/(measure_length*2);

    // reset BG flags to false
    beat_advanced = false;
    shape_advanced = false;

    // locks off loop when lives are 0
    if (game_over == false) {
        if ((current_ticks - beat_start_time) >= length) {

            // plays metronome sounds
            if (get_debug()) {
                if ((beat_count - start_offset)%time_signature_top == 0) {
                    Mix_PlayChannel(1, snd_metronome_big, 0);
                } else {
                    Mix_PlayChannel(1, snd_metronome_small, 0);
                }
            }

            // waits until the song's reached past the intro
            if (song_over == false && beat_count >= start_offset) {

                // triggers at the start of every shape (a.k.a, the start of every CPU/player phase)
                if ((beat_count - start_offset)%(measure_length*2) == 0) {
                    if (beat_count == start_offset) {
                        song_beat_position = beat_count;
                    } else {
                        int song_step_amount = get_song_step((beat_count - (start_offset + 1))/(measure_length*2) + 1, measure_length*2);

                        if (compare_shapes(active_shape, result_shape) == true) {
                            // reward player with life and points
                            modify_life(5);
                            combo++;
                            score += (calculate_score() * combo);

                            // triggers the combo effect when reaching multiples of 5
                            if (combo%5 == 0) {
                                Mix_PlayChannel(-1, snd_combo, 0);
                                set_combo_timer(3000);
                            }

                            // advances the song
                            float timer = (song_beat_position + song_step_amount) * ((60.f/bpm * 2.f) / time_signature_bottom);
                            Mix_SetMusicPosition(timer);
                            song_beat_position += song_step_amount;
                            shape_advanced = true;

                            // pushes shapes to vector for drawing
                            active_shape.color = json_file[shape_count].value("color", 0);
                            previous_shapes.push_back(active_shape);

                            // pushes auto_shapes as well, if present
                            if (json_file[shape_count].contains("auto_shapes") && json_file[shape_count]["auto_shapes"].is_array()) {
                                if (get_debug()) {printf("Pushing auto-shapes to shape draw queue...\n");}

                                for (int i = 0; i < json_file[shape_count]["auto_shapes"].size(); i++) {
                                    shape temp;

                                    temp.type   = json_file[shape_count]["auto_shapes"][i].value("shape", 0);
                                    temp.x      = json_file[shape_count]["auto_shapes"][i].value("x", 7);
                                    temp.y      = json_file[shape_count]["auto_shapes"][i].value("y", 7);
                                    temp.scale  = json_file[shape_count]["auto_shapes"][i].value("scale", 1);
                                    temp.color  = json_file[shape_count]["auto_shapes"][i].value("color", 0);

                                    previous_shapes.push_back(temp);
                                }
                            }
                        } else {
                            // resets the song back to where it goes
                            float timer = song_beat_position * ((60.f/bpm * 2.f) / time_signature_bottom);
                            Mix_SetMusicPosition(timer);
                            beat_count -= measure_length*2;

                            // re-calculates shape_count for cpu_sequence below
                            shape_count = (beat_count - start_offset)/(measure_length*2);

                            // penalize player's life and combo
                            modify_life(-25);
                            combo = 0;
                        }
                    }

                    // are we dead yet?
                    if (life == 0) {
                        printf("Game over!\n");
                        Mix_FadeOutMusic(5000);
                        game_over = true;
                    }

                    reset_shapes();
                    reset_sequences();
                    cpu_sequence = json_file[fmin(shape_count + 1, json_file.size() - 1)].value("sequence", ".");
                }

                // triggers at the start of every swap between CPU and player
                if ((beat_count - start_offset) % measure_length == 0) {
                    reset_character_status();
                }

                // triggers for every beat where the player has control
                if ((beat_count - start_offset) % (measure_length*2) >= measure_length) {
                    if (beat_count%2 == 0) {rumble_controller();}
                }

                // triggers for every beat where the CPU has control
                if ((beat_count - start_offset) % (measure_length*2) < measure_length) {
                    int string_index = (((beat_count) - (start_offset))/(measure_length*2)) + 1;

                    if (string_index <= json_file.size() - 1) {
                        // gets current position in sequence, performs action on shape
                        int index = ((beat_count + 1) - (start_offset + 1)) % measure_length;
                        char current_sequence_pos = cpu_sequence[index];

                        // check to ensure we aren't reading out-of-bounds of the string
                        // also checks to make sure the game isn't over
                        if (index <= measure_length && game_over == false) {
                            result_shape = modify_current_shape(current_sequence_pos, result_shape);
                        }
                    }
                }
            }

            beat_advanced = true;
            beat_count++;
            beat_start_time += length;
        }

        tick_character(frame_time);

        // this basically just says "give us 0 if it's negative, otherwise give us how many shapes have passed"
        // this gets offset by 1 since element 0 in level.json is a header
        shape_count = ((beat_count - start_offset) < 0) ? 0: (beat_count - (start_offset + 1))/(measure_length*2) + 1;

        if (shape_count > json_file.size() - 1) {

            if (song_over == false) {
                printf("End of level reached.\n");
                song_over = true;
                metadata.cleared = true;
            }

            if (shape_count >= json_file.size() + 2 && check_fade_activity() == false) {
                save_metadata();
                printf("Ending level...\n");
                fade_out++;
            }

            shape_count = json_file.size();
        }
    }

    return true;
}

void start_level() {
    Mix_HaltMusic();
    reset_shapes();
    reset_sequences();
    reset_score_and_life();
    reset_character_status();
    unload_character_tileset();

    printf("Loading level: %s\n", get_level_json_path().c_str());
    draw_loading(false);
    load_stage_music();
    load_stage_sound_collection();
    load_character_file();
    init_background_effect();

    printf("Starting level...\n");
}

int main(int argc, char *argv[]) {
    // evaluates certain command-line options before game initialization
    if (parse_option(argv, argv+argc, "-help") || parse_option(argv, argv+argc, "-h")) {print_help(); return 0;}
    if (parse_option(argv, argv+argc, "-log")  || parse_option(argv, argv+argc, "-l")) {freopen("log.txt", "w", stdout);}

    if (!init(argc, argv)) return 1;

    // current_state is self explanatory
    // transition_state stores the next game state during fadeouts
    game_states current_state = WARNING;
    game_states transition_state;
    SDL_Event evt;

    char *opt_startup_level = parse_option_value(argv, argv+argc, "-i");
    if (opt_startup_level) {
        string startup_level = opt_startup_level;

        // Strip json file (if any)
        if (startup_level.substr(startup_level.size() - 5, 5) == ".json") {
            startup_level.erase(startup_level.begin() + startup_level.find_last_of(std::filesystem::path::preferred_separator), startup_level.end());
        }

        // Load the level
        level_paths.push_back(startup_level);
        level_playlists.push_back("");
        json_file = parse_level_file(get_level_json_path());

        start_level();
        previous_shapes.clear();
        transition_state = GAME;
        fade_out = 255;
    } else {
        load_levels();
    }

    // parses arguments related to skipping directly to a given game state
    if (parse_option(argv, argv+argc, "-sandbox")  || parse_option(argv, argv+argc, "-sb")) {
        transition_state = SANDBOX;
        fade_out = 255;
    }

    // used to keep track of what's currently selected in various menus
    int menu_selected = 0;
    int sandbox_option_selected = 0;
    bool sandbox_menu_active = false;
    bool sandbox_quit_dialog_active = false;
    bool sandbox_quit_dialog_selected = false; // only two options, hence bool instead of an int
    bool sandbox_lock = false;

    load_motd();

    // stores values for the FPS counter
    static int start_time, frame_time = 0;
    int fps = 0;
    int time_passed = 0;
    int frame_count = 0;

    bool program_running = true;

    // main loop that runs while the game is active
    while (program_running) {
        start_time = SDL_GetTicks();

        // disables cursor during fullscreen
        if (fullscreen_toggle || true_fullscreen_toggle) {SDL_SetRelativeMouseMode(SDL_TRUE);} else {SDL_SetRelativeMouseMode(SDL_FALSE);}

        // handles all events
        while (SDL_PollEvent(&evt) != 0) {
            switch (evt.type) {
            case SDL_QUIT: program_running = false; break;

            // handles resizing the window
            case SDL_WINDOWEVENT:
                if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_RenderClear(renderer);
                    SDL_GetWindowSize(window, &width, &height);
                }
                break;

            // handles controller connection/disconnection
            case SDL_CONTROLLERDEVICEADDED:
                if (controller == NULL) {
                    printf("Controller connected, initializing...\n");
                    init_controller();
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                SDL_GameControllerClose(controller);
                controller = NULL;
                printf("Controller disconnected.\n");
                break;

            // handles inputs
            case SDL_KEYDOWN:
            case SDL_CONTROLLERBUTTONDOWN:
                controller_buttons input_value;
                bool in_game = false;
                unsigned int timestamp;

                // this flag slightly changes the controls for keyboard to make them feel better in-game
                if (current_state == SANDBOX || current_state == GAME) {in_game = true;}

                // this converts the button presses to an abstract controller which is then used when handling input
                // this reduces code complexity by quite a bit and lets us support both controllers and keyboards more easily
                if (evt.type == SDL_KEYDOWN) {
                    timestamp = evt.key.timestamp;
                    input_value = keyboard_to_abstract_button(evt.key.keysym.sym, in_game);
                }

                if (evt.type == SDL_CONTROLLERBUTTONDOWN) {
                    timestamp = evt.cbutton.timestamp;
                    input_value = gamepad_to_abstract_button(evt.cbutton.button);
                }

                // handles screenshots; not dependent on abstract controller or game state
                if (evt.key.keysym.sym == SDLK_F12) {
                    take_screenshot();
                }

                if (evt.key.keysym.sym == SDLK_F11) {
                    fullscreen_toggle = !fullscreen_toggle;
                    set_fullscreen();
                }

                if (evt.key.keysym.sym == SDLK_F10) {
                    if (get_debug() == true) {printf("Crashing game on purpose...\n"); abort();}
                }

                if (evt.key.keysym.sym == SDLK_F8) {
                    if (get_debug() == true) {save_metadata();}
                    set_fullscreen();
                }

                // THE SWITCH OF DOOM(tm)
                // this controls what the buttons do depending on the current game state
                switch (current_state) {
                    case WARNING:
                        switch(input_value) {
                            case START:
                            if (check_fade_in_activity()) {
                                Mix_PlayChannel(0, snd_menu_confirm, 0);
                                transition_state = TITLE;
                                fade_out++;
                            }
                            break;
                        }
                        break;

                    case TITLE:
                        switch(input_value) {
                            case START:
                            case CROSS:
                            if (check_fade_in_activity()) {
                                Mix_PlayChannel(0, snd_menu_confirm, 0);

                                switch (menu_selected) {
                                    case 0: transition_state =  LEVEL_SELECT;   break;
                                    case 1: transition_state =  SANDBOX;    break;
                                    case 2: transition_state =  TUTORIAL;   break;
                                    case 3: transition_state =  OPTIONS;    break;
                                    case 4: transition_state =  EXIT;   break;
                                }

                                fade_out++;
                            }
                            break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_back, 0);
                                menu_selected = 4;
                                transition_state = EXIT;
                                fade_out++;
                                break;

                            case UP:
                            if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    menu_selected--;
                                }

                            if (menu_selected > 4) {menu_selected = 0;}
                            if (menu_selected < 0) {menu_selected = 4;}

                            break;

                            case DOWN:
                            if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    menu_selected++;
                                }

                            if (menu_selected > 4) {menu_selected = 0;}
                            if (menu_selected < 0) {menu_selected = 4;}

                            break;

                            case TRIANGLE:
                                if (get_debug()) {load_motd();}
                                break;

                        }
                        break;

                    case LEVEL_SELECT:
                        switch(input_value) {
                            case START:
                            case CROSS:
                                if (check_fade_in_activity()) {

                                    if (json_file == NULL) {
                                        if (level_paths.size() == 0) {
                                            printf("Attempting to re-scan the levels folder...\n");
                                            load_levels();

                                            if (level_paths.size() == 0) {break;}
                                        }

                                        printf("Attempting to reload the current file...\n");
                                        json_file = parse_level_file(get_level_json_path());
                                        load_metadata();

                                        break;
                                    }

                                    Mix_PlayChannel(0, snd_menu_confirm, 0);
                                    start_level();
                                    transition_state = GAME;
                                    fade_out++;
                                }
                                break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_back, 0);
                                transition_state = TITLE;
                                fade_out++;
                                break;

                            case LEFT:
                                if (json_file == NULL) {break;}
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    level_index--;
                                    if (level_index < 0) {level_index = level_paths.size() - 1;}
                                    json_file = parse_level_file(get_level_json_path());
                                    load_metadata();
                                }
                                break;

                            case RIGHT:
                                if (json_file == NULL) {break;}
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    level_index++;
                                    if (level_index >= level_paths.size()) {level_index = 0;}
                                    json_file = parse_level_file(get_level_json_path());
                                    load_metadata();
                                }
                                break;
                        }
                        break;

                    case GAME:
                        if (check_fade_activity()) {break;}

                        // disables key repeats from being registered, so a player cant just hold a key to "buffer" inputs
                        // we check for event type because, for some reason, gamepad events will return non-zero for key repeats
                        if (evt.type == SDL_KEYDOWN && evt.key.repeat != 0) {break;}

                        // handles the game over inputs
                        if (game_over) {
                            switch(input_value) {
                                case SELECT:
                                case START:
                                case CIRCLE:
                                case SQUARE:
                                case TRIANGLE:
                                case CROSS:
                                case LB:
                                case RB:
                                    save_play_count();
                                    Mix_PlayChannel(0, snd_menu_confirm, 0);
                                    fade_out++;
                                    break;
                            }
                        } else {
                            // level exiting shouldn't be affected by timing windows
                            int beat_side = check_beat_timing_window(timestamp);
                            char op = '.';

                            if (input_value != SELECT) {
                                // 0 means we're NOT within a timing window
                                if (beat_side == 0) {break;}

                                // this prevents multiple inputs from going through during a single window
                                if (check_available_sequence(beat_side) == false) {break;}
                            }

                            switch(input_value) {
                                case SELECT:
                                    Mix_PlayChannel(0, snd_menu_back, 0);
                                    fade_out++;
                                    break;

                                case UP:
                                    op = 'U';
                                    break;

                                case DOWN:
                                    op = 'D';
                                    break;

                                case LEFT:
                                    op = 'L';
                                    break;

                                case RIGHT:
                                    op = 'R';
                                    break;

                                case CIRCLE:
                                    op = 'Z';
                                    break;

                                case SQUARE:
                                    op = 'X';
                                    break;

                                case TRIANGLE:
                                    op = 'C';
                                    break;

                                case CROSS:
                                    op = 'V';
                                    break;

                                case LB:
                                    op = 'A';
                                    break;

                                case RB:
                                    op = 'S';
                                    break;
                            }

                            set_character_timer(60000/bpm);
                            active_shape    = modify_current_shape(op, active_shape, true);
                            player_sequence = modify_sequence(op, beat_side);
                        }
                        break;

                    case SANDBOX:
                        if (check_fade_activity()) {break;}

                        // inputs for the quit confirmation dialog when exiting sandbox mode
                        if (sandbox_quit_dialog_active) {
                            switch(input_value) {
                                case LEFT:
                                case RIGHT:
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    sandbox_quit_dialog_selected = !sandbox_quit_dialog_selected;
                                    break;

                                case SELECT:
                                    sandbox_quit_dialog_selected = true;
                                    Mix_PlayChannel(0, snd_menu_back, 0);
                                    transition_state = TITLE;
                                    fade_out++;
                                    break;

                                case CROSS:
                                case CIRCLE:
                                case SQUARE:
                                case TRIANGLE:
                                case START:
                                    Mix_PlayChannel(0, snd_menu_back, 0);
                                    if (sandbox_quit_dialog_selected) {
                                        transition_state = TITLE;
                                        fade_out++;
                                    } else {
                                        sandbox_quit_dialog_selected = false;
                                        sandbox_quit_dialog_active = false;
                                    }
                                    break;

                                break;
                            }
                        } else if (sandbox_menu_active) {
                            // inputs for when the sandbox menu is up
                            switch (input_value) {
                                case START:
                                    sandbox_menu_active = !sandbox_menu_active;
                                    break;

                                case LEFT:
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    sandbox_option_selected--;
                                    if (sandbox_option_selected < 0) {sandbox_option_selected = sandbox_item_count - 1;}
                                    break;

                                case RIGHT:
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    sandbox_option_selected++;
                                    if (sandbox_option_selected > sandbox_item_count - 1) {sandbox_option_selected = 0;}
                                    break;

                                case UP:
                                    switch (sandbox_option_selected) {
                                        case 0:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            active_shape.color++;
                                            if (active_shape.color > 16) {active_shape.color = 0;}
                                            break;

                                        default: break;
                                    }
                                    break;

                                case DOWN:
                                    switch (sandbox_option_selected) {
                                        case 0:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            active_shape.color--;
                                            if (active_shape.color < 0) {active_shape.color = 16;}
                                            break;

                                        default: break;
                                    }
                                    break;

                                case CROSS:
                                case CIRCLE:
                                case SQUARE:
                                case TRIANGLE:
                                    switch (sandbox_option_selected) {
                                        case 0:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            active_shape.color++;
                                            if (active_shape.color > 16) {active_shape.color = 0;}
                                            break;

                                        case 1:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            morph_shapes();
                                            break;

                                        case 2:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            morph_colors();
                                            break;

                                        case 3:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            if (previous_shapes.size() > 0) {previous_shapes.pop_back();}
                                            break;

                                        case 4:
                                            Mix_PlayChannel(0, snd_combo, 0);
                                            export_shapes();
                                            break;

                                        case 5:
                                            Mix_PlayChannel(0, snd_xplode, 0);
                                            sandbox_lock = !sandbox_lock;
                                            break;

                                        default: break;
                                    }
                                    break;
                            }
                        } else {
                            // inputs for "normal" sandbox play
                            switch (input_value) {
                                case START:
                                    sandbox_menu_active = !sandbox_menu_active;
                                    break;

                                case SELECT:
                                    sandbox_quit_dialog_active = true;
                                    break;

                                case UP:
                                    active_shape = modify_current_shape('U', active_shape);
                                    break;

                                case DOWN:
                                    active_shape = modify_current_shape('D', active_shape);
                                    break;

                                case LEFT:
                                    active_shape = modify_current_shape('L', active_shape);
                                    break;

                                case RIGHT:
                                    active_shape = modify_current_shape('R', active_shape);
                                    break;

                                case CIRCLE:
                                    active_shape = modify_current_shape('Z', active_shape);
                                    break;

                                case SQUARE:
                                    active_shape = modify_current_shape('X', active_shape);
                                    break;

                                case TRIANGLE:
                                    active_shape = modify_current_shape('C', active_shape);
                                    break;

                                case CROSS:
                                    Mix_PlayChannel(-1, snd_success, 0);
                                    previous_shapes.push_back(active_shape);

                                    if (!sandbox_lock) {
                                        reset_shapes();
                                        active_shape.type = 0;
                                    }

                                    break;

                                case LB:
                                    active_shape = modify_current_shape('A', active_shape);
                                    break;

                                case RB:
                                    active_shape = modify_current_shape('S', active_shape);
                                    break;
                            }
                        }
                        break;

                    case TUTORIAL:
                        switch(input_value) {
                            case SELECT:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_back, 0);
                                transition_state = TITLE;
                                fade_out++;
                                break;

                            case CROSS:
                            case CIRCLE:
                            case SQUARE:
                            case TRIANGLE:
                                if (check_fade_activity()) {break;}

                                tutorial_advance_message();

                                if (check_tutorial_finished()) {
                                    transition_state = TITLE;
                                    fade_out++;
                                }
                                break;
                        }
                        break;

                    case OPTIONS:
                        if (check_rebind()) {
                            if (evt.key.keysym.sym == SDLK_ESCAPE) {
                                printf("Skipping input #%i (%s)\n", get_rebind_index(), get_input_name().c_str());
                                increment_rebind_index();
                            }

                            if (check_rebind_keys() && evt.type == SDL_KEYDOWN && evt.key.keysym.sym != SDLK_ESCAPE) {
                                keymap[get_rebind_index()] = evt.key.keysym.sym;
                                printf("Mapped keyboard key #%i (%s) to: %s\n", get_rebind_index(), get_input_name().c_str(), get_current_mapping().c_str());
                                increment_rebind_index();
                            }

                            if (check_rebind_controller() && evt.type == SDL_CONTROLLERBUTTONDOWN) {
                                buttonmap[get_rebind_index()] = (SDL_GameControllerButton)evt.cbutton.button;
                                printf("Mapped controller button #%i (%s) to: %s\n", get_rebind_index(), get_input_name().c_str(), get_current_mapping().c_str());
                                increment_rebind_index();
                            }

                            if (get_rebind_index() > 11) {
                                reset_rebind_flags();
                            }

                            break;
                        } else switch(input_value) {
                            case START:
                            case CROSS:
                            if (check_fade_in_activity()) {
                                Mix_PlayChannel(0, snd_menu_confirm, 0);

                                if (modify_current_option_button() == 1) {
                                    transition_state = TITLE;
                                    fade_out++;
                                }
                            }
                            break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_back, 0);
                                if (options_back_button() == 1) {
                                    transition_state = TITLE;
                                    fade_out++;
                                }
                                break;

                            case UP:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    move_option_selection(-1);
                                }

                                break;

                            case DOWN:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    move_option_selection(1);
                                }

                                break;

                            case LEFT:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    modify_current_option_directions(-1);
                                }
                                break;

                            case RIGHT:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    modify_current_option_directions(1);
                                }
                                break;

                            case LB:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    modify_current_option_directions(-10);
                                }
                                break;

                            case RB:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    modify_current_option_directions(10);
                                }
                                break;
                        }
                        break;

                }
            }
        }

        // Switches state if applicable
        // Put here so that we can have initialize functions that run at the start of a state
        if (fade_out >= 255) {
            fade_reset();

            // Runs functions at the end of a state
            switch (current_state) {
                case SANDBOX:
                    unload_sandbox_icons();
                    previous_shapes.clear();
                    load_default_music("menu");
                    break;

                case TUTORIAL:
                    load_default_music("menu");
                    break;

                case LEVEL_SELECT:
                    previous_shapes.clear();
                    break;

                case GAME:
                    load_default_music("menu");
                    break;

                case TITLE:
                    unload_logo();
                    break;

                case WARNING:
                    if (transition_state == TITLE) {load_default_music("menu");}
                    break;

                default:
                    break;
            }

            // changes the actual state
            current_state = transition_state;

            // Runs functions at the start of a state
            switch (current_state) {
                case TITLE:
                    load_logo();
                    break;

                case LEVEL_SELECT:
                    if (level_paths.empty()) {
                        json_file = NULL;
                    } else {
                        json_file = parse_level_file(get_level_json_path());
                        load_metadata();
                    }

                    break;

                case SANDBOX:
                    printf("Loading sandbox mode...\n");
                    draw_loading(false);
                    load_default_music("sandbox");
                    load_default_sound_collection();
                    load_sandbox_icons();
                    reset_color_table();
                    reset_shapes();
                    active_shape.type = 0;
                    json_file[0]["background_effect"] = "wave";
                    init_background_effect();
                    sandbox_menu_active = false;
                    sandbox_quit_dialog_active = false;
                    sandbox_quit_dialog_selected = false;
                    sandbox_option_selected = 0;
                    sandbox_lock = false;
                    break;

                case TUTORIAL:
                    printf("Loading tutorial mode...\n");
                    init_tutorial();
                    load_default_music("tutorial");
                    break;

                case OPTIONS:
                    reset_options_menu();
                    break;

                case GAME:
                    song_over = false;
                    game_over = false;
                    transition_state = LEVEL_SELECT;

                    // sets up metronome
                    float one_bar = 60000/bpm;

                    // divides by the number of beats in a bar
                    // this allows for unusual time signatures
                    int time_signature_bottom = get_level_time_signature(false);
                    float bpm_correction = (one_bar*2) / time_signature_bottom;

                    // this marks the start of the song, used for calculating how long the song's been playing
                    // this is used as a global timer for background effects
                    Mix_PlayMusic(music, 0);
                    song_start_time = SDL_GetTicks();

                    beat_start_time = song_start_time - bpm_correction;
                    length = bpm_correction;
                    intro_beat_length = bpm_correction * get_level_intro_delay();
                    beat_count = 0;
                    song_beat_position = 0;

                    break;

                break;
            }
        }

        // Game state handler
        switch (current_state) {
            case WARNING:
                draw_warning(frame_time);
                break;

            case TITLE:
                draw_title(menu_selected, frame_time);
                break;

            case LEVEL_SELECT:
                draw_level_select(previous_shapes, frame_time);
                break;

            case GAME:
                loop(json_file, get_level_intro_delay(), get_level_time_signature(true), get_level_time_signature(false), song_start_time, frame_time);
                draw_game(beat_count, get_level_intro_delay(), get_level_measure_length(), song_start_time, beat_start_time, SDL_GetTicks(), intro_beat_length, beat_advanced, shape_advanced, active_shape, result_shape, previous_shapes, grid_toggle, hud_toggle, blindfold_toggle, song_over, game_over, frame_time);
                break;

            case SANDBOX:
                draw_sandbox(active_shape, previous_shapes, sandbox_menu_active, sandbox_lock, sandbox_option_selected, sandbox_quit_dialog_active, sandbox_quit_dialog_selected, frame_time);
                break;

            case TUTORIAL:
                tutorial_message_tick(frame_time);
                draw_tutorial(frame_time);
                break;

            case OPTIONS:
                draw_options(frame_time);
                break;

            case EXIT:
                program_running = false;
                break;

            default:
                printf("[!] Invalid or unimplemented game state reached!\n");
                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Unimplemented Game State",
                "Open Manifold has entered a game state that doesn't exist or hasn't been implemented yet.\n"
                "If you got here and believe it to be a bug, please report it on our issues page.\n"
                "The game will now close.", window);
                program_running = false;
                break;
        }

        draw_fps(fps_toggle, fps, frame_time);
        SDL_RenderPresent(renderer);

        // calculates FPS
        frame_time = SDL_GetTicks() - start_time;

        // frame capper
        if (!vsync_toggle) {
            if (frame_time < frame_cap_ms) {
                int delay_delta = frame_cap_ms - frame_time;
                SDL_Delay(delay_delta);
                frame_time += delay_delta;
            }
        }

        frame_count++;
        time_passed += frame_time;

        if (time_passed >= 1000) {
            fps = frame_count;
            frame_count = 0;
            time_passed = 0;
        }
    }

    kill();
    return 0;
}
