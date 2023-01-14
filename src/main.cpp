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
#include <cmath>
#include <ctime>
#include <string>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <nlohmann/json.hpp>

#include "graphics.h"
#include "background.h"
#include "character.h"
#include "version.h"

using nlohmann::json;
using std::string;

// basic window stuff
SDL_Window* window;
SDL_Renderer* renderer;
extern int width;
extern int height;

// various options
extern int option_count;
int music_volume = 75;
int sfx_volume = 75;
bool mono_toggle = false;
int frame_cap = 120;
bool fps_toggle = false;
bool fullscreen_toggle = false;
bool true_fullscreen_toggle = false;    // note: not used in the options menu!
bool vsync_toggle = false;
bool grid_toggle = true;
bool debug_toggle = false;
int controller_index = 0;

// main-game variables
int score = 0;
int combo = 0;
int life = 100;

// BG flags
// set to true for one frame when applicable
bool beat_advanced = false;
bool shape_advanced = false;

// timekeeping flags (and some other data)
float beat_start_time;
float length;
int bpm;
int beat_count = 0;
int song_beat_position = 0;
int song_start_time;
int intro_beat_length;
string cpu_sequence;
string player_sequence;
bool song_over = false;
bool game_over = false;
background_effect background_id;

// sound effects
Mix_Chunk *snd_menu_move;
Mix_Chunk *snd_menu_confirm;
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

// the music track that's playing
Mix_Music *music;

// in addition to actually drawing the fade, these are also used to handle game-state transitions
extern float fade_in;
extern float fade_out;

// used to keep track of what options are selected in menus
int menu_selected = 0;
int option_selected = 0;

// stores file paths
std::vector<string> level_paths;   // e.g. "assets/levels/Foo Bar"
int level_index = 0;

// contains the currently-loaded JSON level data
json json_file;

// stores current list of shapes and active shape for main game
// previous_shapes is all the previous shapes in order
// active shape is the one the CPU/player manipulates
// result_shape is compared to at the end of a measure to see if a shape matches
std::vector<shape> previous_shapes;
shape active_shape;
shape result_shape;

// all the possible game states
enum game_states {
    WARNING,
    TITLE,
    STAGE_SELECT,
    GAME,
    SANDBOX,
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

// the controller, if one is needed
SDL_GameController *controller;

// message of the day string
string motd = "";

bool parse_option(char** argc, char** argv, const string& opt) {
    return std::find(argc, argv, opt) != argv;
}

void print_header() {
    printf("OPEN MANIFOLD v%i.%i.%i\nBuild Date: %s at %s\n========================================\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, __DATE__, __TIME__);
    return;
}

void print_help() {
    print_header();
    printf("\nAccepted parameters are:\n\n"
    "-h  / -help            - Print this message\n"
    "-l  / -log             - Writes a log to file\n"
    "-f  / -fullscreen      - Enable fullscreen\n"
    "-tf / -true-fullscreen - Enable 'real' fullscreen\n"
    "-v  / -vsync           - Enable V-Sync\n"
    "-d  / -debug           - Enable debug features\n");
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
    
    // convert current keymap into string array
    string keymap_strings[12];
    
    for (int i = 0; i < 12; i++) {
        keymap_strings[i] = SDL_GetKeyName(keymap[i]);
    }

    new_config["music_volume"] = music_volume;
    new_config["sfx_volume"] = sfx_volume;
    new_config["mono_toggle"] = mono_toggle;
    new_config["display_fps"] = fps_toggle;
    new_config["fullscreen"] = fullscreen_toggle;
    new_config["vsync"] = vsync_toggle;
    new_config["frame_cap"] = frame_cap;
    new_config["display_grid"] = grid_toggle;
    new_config["controller_index"] = controller_index;
    new_config["key_map"] = keymap_strings;

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
    if (json_data.contains("display_fps"))      {fps_toggle = json_data["display_fps"];}
    if (json_data.contains("fullscreen"))       {fullscreen_toggle = json_data["fullscreen"];}
    if (json_data.contains("vsync"))            {vsync_toggle = json_data["vsync"];}
    if (json_data.contains("frame_cap"))        {frame_cap = json_data["frame_cap"];}
    if (json_data.contains("display_grid"))     {grid_toggle = json_data["display_grid"];}
    if (json_data.contains("music_volume"))     {music_volume = json_data["music_volume"];}
    if (json_data.contains("sfx_volume"))       {sfx_volume = json_data["sfx_volume"];}
    if (json_data.contains("mono_toggle"))      {mono_toggle = json_data["mono_toggle"];}
    if (json_data.contains("controller_index")) {controller_index = json_data["controller_index"];}
    
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
    const std::filesystem::path levels{"assets/levels"};
    unsigned int scanned_level_count = 0; // unsigned since a negative level count makes no sense

    printf("Scanning for levels...\n");

    if (!std::filesystem::is_directory(levels)) {
        printf("[!] The levels directory (assets/levels) couldn't be found!\n");
        return;
    }

    for (auto& dir_entry: std::filesystem::directory_iterator{levels}) {

        // check: is this entry a directory?
        if (std::filesystem::is_directory(dir_entry.path())) {
            std::filesystem::path level_subdirectory(dir_entry.path());

            // checks for the existence of a level.json file within that directory
            for (auto& subdir_entry: std::filesystem::directory_iterator{level_subdirectory}) {
                string filename = subdir_entry.path().filename().string();

                if (filename == "level.json") {
                    string level_path = dir_entry.path().string();
                    level_paths.push_back(level_path);
                    scanned_level_count++;
                    
                    printf("Detected level: %s\n", level_path.c_str());
                    break;
                }
            }
        }
    }

    if (scanned_level_count == 0) {
        printf("[!] No levels were found!\n");
    } else {
        printf("Found %i levels.\n", scanned_level_count);
    }
    
    // sorts the level lists alphabetically (std::filesystem does not guarantee this)
    std::sort(level_paths.begin(), level_paths.end());

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

void set_vsync_renderer() {
    // sets VSYNC flag in SDL2 and re-creates the renderer
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, std::to_string(vsync_toggle).c_str());

    SDL_DestroyRenderer(renderer);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    if (renderer == NULL) {
        printf("[!] Error re-creating renderer: %s\n", SDL_GetError());
    }

    // need to reload font texture as well, since destroying the renderer also destroys textures
    load_font();
    return;
}

void take_screenshot() {
    // get current time for filename
    time_t rawtime;
    struct tm *timeinfo;
    char filename[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // construct filename
    strftime(filename, sizeof(filename), "%Y-%m-%d_%H-%M-%S.png", timeinfo);

    printf("Saving screenshot: %s\n", filename);

    // take the screenshot and save it as PNG
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch);
    IMG_SavePNG(surface, filename);
    SDL_FreeSurface(surface);
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

    switch (input) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP:         return UP;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:       return DOWN;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:       return LEFT;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:      return RIGHT;
        case SDL_CONTROLLER_BUTTON_A:               return CROSS;
        case SDL_CONTROLLER_BUTTON_B:               return CIRCLE;
        case SDL_CONTROLLER_BUTTON_X:               return SQUARE;
        case SDL_CONTROLLER_BUTTON_Y:               return TRIANGLE;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:    return LB;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:   return RB;
        case SDL_CONTROLLER_BUTTON_START:           return START;
        case SDL_CONTROLLER_BUTTON_BACK:            return SELECT;
    }

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
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, std::to_string(vsync_toggle).c_str());

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
    if (fullscreen_toggle) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }

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

    // loads some sound effects
    printf("Loading common sound effects...\n");
    snd_menu_move = Mix_LoadWAV("assets/sound/move.ogg");
    if(snd_menu_move == NULL) {
        printf("[!] %s\n", Mix_GetError());
    }

    snd_menu_confirm = Mix_LoadWAV("assets/sound/confirm.ogg");
    if(snd_menu_confirm == NULL) {
        printf("[!] %s\n", Mix_GetError());
    }

    snd_metronome_small = Mix_LoadWAV("assets/sound/metronome_small.ogg");
    if(snd_metronome_small == NULL) {
        printf("[!] %s\n", Mix_GetError());
    }

    snd_metronome_big = Mix_LoadWAV("assets/sound/metronome_big.ogg");
    if(snd_metronome_big == NULL) {
        printf("[!] %s\n", Mix_GetError());
    }

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

background_effect get_level_background_effect() {
    // converts a background_effect's value to an enum used internally
    // we do this for readability, and because comparing ints are faster than comparing strings

    string background_name = json_file[0].value("background_effect", "none");

    if (background_name == "solid")         return solid;
    if (background_name == "tile")          return tile;
    if (background_name == "checkerboard")  return checkerboard;
    if (background_name == "fire")          return fire;
    if (background_name == "conway")        return conway;
    if (background_name == "monitor")       return monitor;
    if (background_name == "wave")          return wave;
    if (background_name == "starfield")     return starfield;
    if (background_name == "hexagon")       return hexagon;
    if (background_name == "munching")      return munching;

    return none;
}

bool get_debug() {
    return debug_toggle;
}

int calculate_score() {
    // calculates a score to give the player by comparing sequence strings
    // ----------------------------------------------------------
    // SCORE TABLE IS: 100 for any ops that match CPU (except blanks), 50 for new ops, 25 for new xplodes
    int score = 0;
    string cpu_sequence = get_cpu_sequence();
    string player_sequence = get_player_sequence();
    
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

json parse_level_file(string file) {
    // loads a JSON file and parses it, filling in blanks if needed; called every time a new level is hovered over via level select
    // ----------------------------------------------------------
    // file: a path to a file, usually supplied by get_level_json_path() ("assets/levels/foobar/level.json")

    if (get_debug()) {printf("Parsing level file: %s\n", file.c_str());}

    std::ifstream ifs(file);
    json parsed_json;

    // checks to make sure the file exists
    if (std::filesystem::exists(file) == false) {
        printf("[!] Couldn't parse level file: Does not appear to exist, despite being indexed\n");
        printf("Either the file was deleted after scanning, or something has gone VERY wrong.\n");
        bpm = get_level_bpm();
        return NULL;
    }

    // checks to see if the JSON is valid JSON
    try {
        parsed_json = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Error parsing level file: %s\n", err.what());
        bpm = get_level_bpm();
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
    int max_sequence_length = get_level_measure_length();

    for (int i = 1; i < parsed_json.size(); i++) {
        int shape = parsed_json[i].value("shape", 0);
        int x = parsed_json[i].value("x", 7);
        int y = parsed_json[i].value("y", 7);
        int scale = parsed_json[i].value("scale", 1);

        bool sequence_exists = parsed_json[i].contains("sequence");
        string generated_sequence;

        // sets the first action to changing shape
        switch (shape) {
            case 0: generated_sequence += "Z"; break;
            case 1: generated_sequence += "X"; break;
            case 2: generated_sequence += "C"; break;
            default: generated_sequence += "Z"; break;
        }

        // scales up the shape to the needed size
        if (scale > 1) {
            for (int j = 1; j < scale; j++) {generated_sequence += "S";}
        }

        // moves across the X axis
        if (x > 7) {
            for (int j = 7; j < x; j++) {generated_sequence += "R";}
        }

        if (x < 7) {
            for (int j = 7; j > x; j--) {generated_sequence += "L";}
        }

        // moves across the Y axis
        if (y > 7) {
            for (int j = 7; j < y; j++) {generated_sequence += "D";}
        }

        if (y < 7) {
            for (int j = 7; j > y; j--) {generated_sequence += "U";}
        }

        // checks to see if the generated_sequence can fit within the allotted number of beats
        if (generated_sequence.length() > max_sequence_length) {
            printf("[!] Generated sequence is longer than max number of beats! Level is not winnable.\n");
        }

        // pads the sequence with NOPs
        if (generated_sequence.length() < max_sequence_length) {
            generated_sequence.insert(generated_sequence.end(), max_sequence_length - generated_sequence.length(), '.');
        }

        if (get_debug()) {printf("g_seq: %s\n", generated_sequence.c_str());}

        // check if a sequence is defined for this shape, if not, use the generated one
        // checking this AFTER making a sequence is inefficient, but also makes it much
        // more likely to catch impossible levels and is negligible on performance
        if (sequence_exists) {continue;} else {
            parsed_json[i]["sequence"] = generated_sequence;
        }
    }

    return parsed_json;
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

void load_menu_music() {
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    music = Mix_LoadMUS("assets/music/menu.ogg");

    if(music == NULL) {
        printf("%s\n", Mix_GetError());
    } else {
        Mix_PlayMusic(music, -1);
    }
    return;
}

void load_sandbox_music() {
    Mix_HaltMusic();
    Mix_FreeMusic(music);
    music = Mix_LoadMUS("assets/music/sandbox.ogg");

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

    string mus_temp = level_paths[level_index] + "/song.ogg";
    const char* mus = mus_temp.c_str();

    music = Mix_LoadMUS(mus);
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

void load_stage_sound_collection() {
    // wrapper that loads every sound a level can use aside from music
    // uses the load_stage_sound function below

    // frees everything, this prevents a memleak from loading tons of levels in one session
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

    return;
}

void load_default_sound_collection() {
    // similar to above but specifically for default sounds
    // this is used by the sandbox mode

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

    return;
}

bool check_fade_activity() {
    // returns true if either fade value is anything other than 0
    // in other words, this is true as long as any fade is happening
    if (fade_in != 0 || fade_out != 0) {return true;} else return false;
}

bool check_fade_in_activity() {
    // similar to check_fade_activity, but allows fade_in to be active
    if ((fade_in <= 1) && (fade_out == 0)) {return true;} else return false;
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

int check_beat_timing_window() {
    // returns 0 if we're not within the beat, 1 on the "left side" (beat end), 2 on the right side (beat start)
    // the window should be within ~120 ms, or 60ms on each "side" of a beat (roughly equivalent to a "Good" in DDR)

    int current_time = SDL_GetTicks();
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

bool compare_shapes() {
    // returns true if the player and CPU shapes match properties

    if (result_shape.x != active_shape.x) return false;
    if (result_shape.y != active_shape.y) return false;
    if (result_shape.type != active_shape.type) return false;
    if (result_shape.scale != active_shape.scale) return false;

    return true;
}

shape modify_current_shape(char opcode, shape current_shape, bool is_player = false) {
    // modifies the shape passed into it depending on an internal "opcode"
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
            Mix_PlayChannel(-1, snd_circle, 0);
            modified_shape.type = 0;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // square
        case 'X':
            Mix_PlayChannel(-1, snd_square, 0);
            modified_shape.type = 1;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // triangle
        case 'C':
            Mix_PlayChannel(-1, snd_triangle, 0);
            modified_shape.type = 2;
            modified_shape.x = 7;
            modified_shape.y = 7;
            modified_shape.scale = 1;
            break;

        // x-plode (essentially a NOP)
        case 'V':
            Mix_PlayChannel(-1, snd_xplode, 0);
            break;

        // shrink
        case 'A':
            Mix_PlayChannel(-1, snd_scale_down, 0);
            modified_shape.scale = fmax(1, modified_shape.scale - 1);
            break;

        // grow
        case 'S':
            Mix_PlayChannel(-1, snd_scale_up, 0);
            modified_shape.scale = fmin(8, modified_shape.scale + 1);
            break;

        // up
        case 'U':
            Mix_PlayChannel(-1, snd_up, 0);
            modified_shape.y = fmax(modified_shape.y - 1, 0);
            break;

        // down
        case 'D':
            Mix_PlayChannel(-1, snd_down, 0);
            modified_shape.y = fmin(modified_shape.y + 1, 14);
            break;

        // left
        case 'L':
            Mix_PlayChannel(-1, snd_left, 0);
            modified_shape.x = fmax(modified_shape.x - 1, 0);
            break;

        // right
        case 'R':
            Mix_PlayChannel(-1, snd_right, 0);
            modified_shape.x = fmin(modified_shape.x + 1, 14);
            break;

        // no-op
        case '.':
            break;

        default: break;
    }

    return modified_shape;
}

bool check_available_sequence(int beat_side) {
    // returns true if the current sequence op is blank, false if it isn't
    // this ensures that both a sequence can't be overwritten and that only
    // ONE input is registered, preventing "diagonals" for example
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
    
                        if (compare_shapes() == true) {
                            // reward player with life and points
                            modify_life(5);
                            combo++;
                            score += (calculate_score() * combo);
                            
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
                    if (beat_count%2 == 0) {SDL_GameControllerRumble(controller, 0xFFFF, 0xFFFF, 120);}
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

        // this basically just says "give us 0 if it's negative, otherwise give us how many shapes have passed"
        // this gets offset by 1 since element 0 in level.json is a header
        shape_count = ((beat_count - start_offset) < 0) ? 0: (beat_count - (start_offset + 1))/(measure_length*2) + 1;
    
        if (shape_count > json_file.size() - 1) {
    
            if (song_over == false) {
                printf("End of level reached.\n");
                song_over = true;
            }
    
            if (shape_count >= json_file.size() + 2 && check_fade_activity() == false) {
                printf("Ending level...\n");
                fade_out++;
            }
    
            shape_count = json_file.size();
        }
    }

    return true;
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

    load_levels();
    load_motd();

    // stores values for the FPS counter
    static int start_time, frame_time = 0;
    int fps = 0;
    int time_passed = 0;
    int frame_count = 0;
    int frame_cap_ms = (1000 / frame_cap);

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

                // this flag slightly changes the controls for keyboard to make them feel better in-game
                if (current_state == SANDBOX || current_state == GAME) {in_game = true;}

                // this converts the button presses to an abstract controller which is then used when handling input
                // this reduces code complexity by quite a bit and lets us support both controllers and keyboards more easily
                if (evt.type == SDL_KEYDOWN) {input_value = keyboard_to_abstract_button(evt.key.keysym.sym, in_game);}
                if (evt.type == SDL_CONTROLLERBUTTONDOWN) {input_value = gamepad_to_abstract_button(evt.cbutton.button);}

                // handles screenshots; not dependent on abstract controller or game state
                if (evt.key.keysym.sym == SDLK_F12) {
                    take_screenshot();
                }
                
                if (evt.key.keysym.sym == SDLK_F10) {
                    if (get_debug() == true) {printf("Crashing game on purpose...\n"); abort();}
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
                                    case 0: transition_state =  STAGE_SELECT;   break;
                                    case 1: transition_state =  SANDBOX;    break;
                                    case 2: transition_state =  OPTIONS;    break;
                                    case 3: transition_state =  EXIT;   break;
                                }

                                fade_out++;
                            }
                            break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_confirm, 0);
                                menu_selected = 3;
                                transition_state = EXIT;
                                fade_out++;
                                break;

                            case UP:
                            if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    menu_selected--;
                                }

                            if (menu_selected > 3) {menu_selected = 0;}
                            if (menu_selected < 0) {menu_selected = 3;}

                            break;

                            case DOWN:
                            if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    menu_selected++;
                                }

                            if (menu_selected > 3) {menu_selected = 0;}
                            if (menu_selected < 0) {menu_selected = 3;}

                            break;

                            case TRIANGLE:
                                if (get_debug()) {load_motd();}
                                break;

                        }
                        break;

                    case STAGE_SELECT:
                        switch(input_value) {
                            case START:
                            case CROSS:
                                if (json_file == NULL) {break;}
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_confirm, 0);
                                    Mix_HaltMusic();
                                    reset_shapes();
                                    reset_sequences();
                                    reset_score_and_life();
                                    reset_character_status();
                                    unload_character_tileset();

                                    printf("Loading level: %s\n", get_level_json_path().c_str());
                                    draw_loading(false);
                                    load_stage_sound_collection();
                                    load_stage_music();
                                    load_character_file();
                                    background_id = get_level_background_effect();
                                    init_background_effect(background_id);

                                    printf("Starting level...\n");
                                    transition_state = GAME;
                                    fade_out++;
                                }
                                break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_confirm, 0);
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
                                }
                                break;

                            case RIGHT:
                                if (json_file == NULL) {break;}
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    level_index++;
                                    if (level_index >= level_paths.size()) {level_index = 0;}
                                    json_file = parse_level_file(get_level_json_path());
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
                                    Mix_PlayChannel(0, snd_menu_confirm, 0);
                                    fade_out++;
                                    break;
                            }
                        } else {
                            // level exiting shouldn't be affected by timing windows
                            int beat_side = check_beat_timing_window();
                            char op = '.';
                            
                            if (input_value != SELECT) {
                                // 0 means we're NOT within a timing window
                                if (beat_side == 0) {break;}
                                
                                // this prevents multiple inputs from going through during a single window
                                if (check_available_sequence(beat_side) == false) {break;}
                            }
                            
                            switch(input_value) {
                                case SELECT:
                                    Mix_PlayChannel(0, snd_menu_confirm, 0);
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
                            
                            active_shape    = modify_current_shape(op, active_shape, true);
                            player_sequence = modify_sequence(op, beat_side);
                        }
                        break;

                    case SANDBOX:
                        if (check_fade_activity()) {break;}

                        switch(input_value) {
                            case START:
                                Mix_PlayChannel(-1, snd_success, 0);
                                previous_shapes.push_back(active_shape);
                                reset_shapes();
                                active_shape.type = 0;
                                break;

                            case SELECT:
                                Mix_PlayChannel(0, snd_menu_confirm, 0);
                                transition_state = TITLE;
                                fade_out++;
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
                                active_shape = modify_current_shape('V', active_shape);
                                active_shape.color = (active_shape.color + 1)%17;
                                break;

                            case LB:
                                active_shape = modify_current_shape('A', active_shape);
                                break;

                            case RB:
                                active_shape = modify_current_shape('S', active_shape);
                                break;
                        }
                        break;

                    case OPTIONS:
                        switch(input_value) {
                            case START:
                            case CROSS:
                            if (check_fade_in_activity()) {
                                Mix_PlayChannel(0, snd_menu_confirm, 0);

                                switch (option_selected) {
                                    case 2: mono_toggle = !mono_toggle; 
                                            set_channel_mix(); 
                                            break;
                                    
                                    case 3: fps_toggle = !fps_toggle; break;

                                    case 4: vsync_toggle = !vsync_toggle;
                                            set_vsync_renderer();
                                            break;

                                    case 6: fullscreen_toggle = !fullscreen_toggle;
                                            if (fullscreen_toggle) {SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);}
                                            else {SDL_SetWindowFullscreen(window, 0);}
                                            break;

                                    case 7: grid_toggle = !grid_toggle; break;

                                    case 9: save_settings();
                                            // the lack of break here is deliberate

                                    case 10: transition_state = TITLE;
                                            fade_out++;
                                            break;

                                    default: break;
                                }
                            }
                            break;

                            case SELECT:
                            case CIRCLE:
                                if (check_fade_activity()) {break;}
                                Mix_PlayChannel(0, snd_menu_confirm, 0);
                                transition_state = TITLE;
                                fade_out++;
                                break;

                            case UP:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    option_selected--;
                                }

                                if (option_selected > option_count - 1) {option_selected = 0;}
                                if (option_selected < 0) {option_selected = option_count - 1;}

                                break;

                            case DOWN:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    option_selected++;
                                }

                                if (option_selected > option_count - 1) {option_selected = 0;}
                                if (option_selected < 0) {option_selected = option_count - 1;}

                                break;

                            case LEFT:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    switch (option_selected){
                                        case 0: music_volume = fmax(0, music_volume - 1); set_music_volume(); break;
                                        case 1: sfx_volume = fmax(0, sfx_volume - 1); set_sfx_volume(); break;
                                        case 5: frame_cap = fmax(30, frame_cap - 1); frame_cap_ms = (1000 / frame_cap); break;
                                        case 8: {
                                            int max_gamepad_index = SDL_NumJoysticks() - 1;
                                            controller_index--;
            
                                            if (controller_index > max_gamepad_index) {controller_index = 0;}
                                            if (controller_index < 0) {controller_index = max_gamepad_index;}
                                            if (max_gamepad_index <= 0) {controller_index = 0;}
                                            init_controller();
                                            break;
                                        }
                                        default: break;
                                    }
                                }
                                break;

                            case RIGHT:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    switch (option_selected){
                                        case 0: music_volume = fmin(100, music_volume + 1); set_music_volume(); break;
                                        case 1: sfx_volume = fmin(100, sfx_volume + 1); set_sfx_volume(); break;
                                        case 5: frame_cap = fmin(1000, frame_cap + 1); frame_cap_ms = (1000 / frame_cap); break;
                                        case 8: {
                                            int max_gamepad_index = SDL_NumJoysticks() - 1;
                                            controller_index++;
            
                                            if (controller_index > max_gamepad_index) {controller_index = 0;}
                                            if (controller_index < 0) {controller_index = max_gamepad_index;}
                                            if (max_gamepad_index <= 0) {controller_index = 0;}
                                            init_controller();
                                            break;
                                        }
                                        default: break;
                                    }
                                }
                                break;

                            case LB:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    switch (option_selected){
                                        case 0: music_volume = fmax(0, music_volume - 10); set_music_volume(); break;
                                        case 1: sfx_volume = fmax(0, sfx_volume - 10); set_sfx_volume(); break;
                                        case 5: frame_cap = fmax(30, frame_cap - 10); frame_cap_ms = (1000 / frame_cap); break;
                                        default: break;
                                    }
                                }
                                break;

                            case RB:
                                if (check_fade_in_activity()) {
                                    Mix_PlayChannel(0, snd_menu_move, 0);
                                    switch (option_selected){
                                        case 0: music_volume = fmin(100, music_volume + 10); set_music_volume(); break;
                                        case 1: sfx_volume = fmin(100, sfx_volume + 10); set_sfx_volume(); break;
                                        case 5: frame_cap = fmin(1000, frame_cap + 10); frame_cap_ms = (1000 / frame_cap); break;
                                        default: break;
                                    }
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
                case GAME:
                    previous_shapes.clear();
                    load_menu_music();
                    break;

                case TITLE:
                    unload_logo();
                    break;

                case WARNING:
                    load_menu_music();
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

                case STAGE_SELECT:
                    if (level_paths.empty()) {
                        json_file = NULL;
                    } else {
                        json_file = parse_level_file(get_level_json_path());
                    }

                    break;

                case SANDBOX:
                    printf("Loading sandbox mode...\n");
                    draw_loading(false);
                    load_sandbox_music();
                    load_default_sound_collection();
                    reset_color_table();
                    reset_shapes();
                    active_shape.type = 0;
                    background_id = wave;
                    init_background_effect(background_id);
                    break;

                case GAME:
                    song_over = false;
                    game_over = false;
                    transition_state = STAGE_SELECT;

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

            case STAGE_SELECT:
                draw_level_select(json_file, frame_time);
                break;

            case GAME:
                loop(json_file, get_level_intro_delay(), get_level_time_signature(true), get_level_time_signature(false), song_start_time, frame_time);
                draw_game(beat_count, get_level_intro_delay(), get_level_measure_length(), song_start_time, beat_start_time, SDL_GetTicks(), intro_beat_length, beat_advanced, shape_advanced, background_id, active_shape, result_shape, previous_shapes, grid_toggle, song_over, game_over, frame_time);
                break;

            case SANDBOX:
                draw_sandbox(background_id, active_shape, previous_shapes, frame_time);
                break;

            case OPTIONS:
                draw_options(option_selected, music_volume, sfx_volume, mono_toggle, frame_cap, fps_toggle, vsync_toggle, fullscreen_toggle, grid_toggle, controller_index, frame_time);
                break;

            case EXIT:
                program_running = false;
                break;
        }

        // calculate frame time manually, separate from frame_time
        draw_fps(fps_toggle, fps, SDL_GetTicks() - start_time);
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