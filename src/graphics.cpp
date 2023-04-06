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
#include <string>
#include <regex>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <nlohmann/json.hpp>

#include "main.h"
#include "character.h"
#include "background.h"
#include "options.h"
#include "font.h"

using nlohmann::json;
using std::string;

extern SDL_Window* window;
extern SDL_Renderer* renderer;

const float to_rad = 3.1415926535 / 180;

// dedicated struct for a shape, saves some time over using a JSON array
// doesn't contain sequence data
struct shape {
    int type;
    int x;
    int y;
    int scale;
    int color;
};

int width  = 1280;
int height = 720;
float fade_in  = 255;
float fade_out = 0;
int combo_display_timer = 0;

SDL_Surface* font;
SDL_Texture* font_texture;
SDL_Texture* logo_texture;
SDL_Texture* sandbox_icon_texture;

// for other character data (rects), see character.cpp
SDL_Texture* char_texture;

// arbitrary variables used in background effects
// has no specific usage, so its used in a LOT of different ways on a per-effect basis
SDL_Texture* aux_texture;
int aux_texture_w;
int aux_texture_h;
bool aux_bool_array[32][32];
int aux_int;
float aux_float;

// default color table; used as a failsafe if color_table entries are invalid/nonexistent
// palette is slightly modified from the CGA 16-color palette (dark yellow is orange, light yellow is regular yellow)
// see https://en.wikipedia.org/wiki/Color_Graphics_Adapter#Color_palette
// (also, note the lack of color #17 here and in color_table, that's used for erase and SHOULDN'T be overwritten)
const SDL_Color default_color_table[16] = {
    {255, 255, 255, 255},
    {0, 0, 255, 255},
    {0, 255, 0, 255},
    {0, 255, 255, 255},
    {255, 0, 0, 255},
    {255, 0, 255, 255},
    {255, 100, 0, 255},
    {192, 192, 192, 255},
    {100, 100, 100, 255},
    {100, 100, 255, 255},
    {100, 255, 100, 255},
    {100, 255, 255, 255},
    {255, 100, 100, 255},
    {255, 100, 255, 255},
    {255, 255, 16, 255},
    {0, 0, 0, 255}
};

// "live" color table, this is the one thats used by get_color() below
SDL_Color color_table[16] = {
    {255, 255, 255, 255},
    {0, 0, 255, 255},
    {0, 255, 0, 255},
    {0, 255, 255, 255},
    {255, 0, 0, 255},
    {255, 0, 255, 255},
    {255, 100, 0, 255},
    {192, 192, 192, 255},
    {100, 100, 100, 255},
    {100, 100, 255, 255},
    {100, 255, 100, 255},
    {100, 255, 255, 255},
    {255, 100, 100, 255},
    {255, 100, 255, 255},
    {255, 255, 16, 255},
    {0, 0, 0, 255}
};

// used for the tile BGFX
std::vector<SDL_Rect> tile_frames = {
    {0, 0, 0, 0}
};

extern int option_count;

// similar data for the sandbox menu
// TODO: split off this (and other sandbox functions) into their own file
const char* sandbox_items[] = {
    "Change Color",
    "Shape Morph",
    "Color Morph",
    "Undo Last Shape",
    "Export to JSON"
};

int sandbox_item_count = std::size(sandbox_items);

void unload_logo() {
    SDL_DestroyTexture(logo_texture);
    return;
}

void unload_sandbox_icons() {
    SDL_DestroyTexture(sandbox_icon_texture);
    return;
}

void unload_character_tileset() {
    SDL_DestroyTexture(char_texture);
    return;
}

void load_background_tileset() {
    string tile_path = get_background_tile_path();
    SDL_Surface* temp = IMG_Load(tile_path.c_str());

    if (temp == NULL) {
        printf("[!] %s\n"
        "Generating placeholder tile texture...\n", SDL_GetError());
        aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, 4, 1);

        // fill texture with pure-black pixels
        SDL_SetRenderTarget(renderer, aux_texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
    } else {
        printf("Loaded background image: %s\n", tile_path.c_str());
        aux_texture = SDL_CreateTextureFromSurface(renderer, temp);
    }

    SDL_FreeSurface(temp);
    return;
}

void load_character_tileset() {
    string tile_path = get_character_tile_path();
    SDL_Surface* temp = IMG_Load(tile_path.c_str());

    if (temp == NULL) {
        printf("[!] %s\n", SDL_GetError());
        char_texture = NULL;
    } else {
        printf("Loaded character image: %s\n", tile_path.c_str());
        char_texture = SDL_CreateTextureFromSurface(renderer, temp);
    }

    SDL_FreeSurface(temp);
    SDL_SetTextureScaleMode(char_texture, SDL_ScaleModeLinear);
    return;
}

void load_logo() {
    SDL_Surface* temp = IMG_Load("assets/logo.png");

    if (temp == NULL) {
        printf("[!] %s\n", SDL_GetError());
        logo_texture = NULL;
    } else {
        logo_texture = SDL_CreateTextureFromSurface(renderer, temp);
    }

    SDL_FreeSurface(temp);
    SDL_SetTextureScaleMode(logo_texture, SDL_ScaleModeLinear);
    return;
}

void load_sandbox_icons() {
    SDL_Surface* temp = IMG_Load("assets/sandbox_icons.png");

    if (temp == NULL) {
        printf("[!] %s\n"
        "Sandbox icons will be blank!\n", SDL_GetError());
        sandbox_icon_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 6, 1);

        // fill texture with pure-black pixels
        SDL_SetRenderTarget(renderer, sandbox_icon_texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
    } else {
        printf("Loaded sandbox icons.\n");
        sandbox_icon_texture = SDL_CreateTextureFromSurface(renderer, temp);
    }

    SDL_FreeSurface(temp);
    SDL_SetTextureScaleMode(sandbox_icon_texture, SDL_ScaleModeLinear);
    return;
}

void load_fallback_font() {
    // this is called if, for whatever reason, assets/font.png isn't found
    // loads pixel data from font.h directly into the font surface
    // implementation courtesy of https://blog.gibson.sh/2015/04/13/how-to-integrate-your-sdl2-window-icon-or-any-image-into-your-executable/
    
    Uint32 rmask, gmask, bmask, amask;
    
    // set color masks based on SDL endianness
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    } else {
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    }
    
    font = SDL_CreateRGBSurfaceFrom((void*)fallback_font.pixel_data, fallback_font.width, fallback_font.height, fallback_font.bytes_per_pixel*8, fallback_font.bytes_per_pixel*fallback_font.width, rmask, gmask, bmask, amask);
    return;
}

void load_font() {
    SDL_FreeSurface(font);
    SDL_DestroyTexture(font_texture);
    
    font = IMG_Load("assets/font.png");

    if (font == NULL) {
        printf("[!] %s\nLoading fallback font...\n", SDL_GetError());
        load_fallback_font();
    }

    font_texture = SDL_CreateTextureFromSurface(renderer, font);
    return;
}

void fallback_tile_frames() {
    // generates a fallback vector if parse_tile_frames fails
    // or if the provided tile.json doesn't exist or is invalid
    
    printf("Using fallback data for tile frames...\n");
    std::vector<SDL_Rect> data;
    
    for (int i = 0; i < aux_texture_w; i += aux_texture_h) {
        SDL_Rect temp_rect;
        int width = aux_texture_h;
        
        // clamp the width of the last frame to not exceed image bounds
        if (i + aux_texture_h > aux_texture_w) {
            width = aux_texture_w - (i-1 * aux_texture_h);
        }
        
        temp_rect.y = 0;
        temp_rect.x = i;
        temp_rect.h = aux_texture_h;
        temp_rect.w = width;
        
        data.push_back(temp_rect);
    }
    
    tile_frames = data;
    return;
}

void parse_tile_frames(json file) {
    // converts the JSON data from tile.json into SDL_Rects
    
    std::vector<SDL_Rect> data;
    
    // TODO: parse header parameters (that dont exist yet)
    
    // note the 1; array entry 0 is a header, like with levels
    for (int i = 1; i < file.size(); i++) {
        SDL_Rect temp_rect;

        temp_rect.x = file[i].value("x", 0);
        temp_rect.y = file[i].value("y", 0);
        temp_rect.w = file[i].value("w", 0);
        temp_rect.h = file[i].value("h", 0);
        
        data.push_back(temp_rect);
    }
    
    if (data.size() == 0) {
        fallback_tile_frames();
        return;
    } else {
        tile_frames = data;
    }
    
    return;
}

SDL_Color hex_string_to_color(string string) {
    // converts a hex-color string into an SDL_Color
    // used for creating color table values for shapes
    // should accept both three-digit and six-digit codes, hex-sign required
    // e.g. #fff & #7289da are valid, but fff & 7289da are not

    // returns this if the string turns out to be invalid/malformed
    SDL_Color error_color = {255, 0, 255, 255};
    SDL_Color color;

    const std::regex validate_hex_code ("^#([0-9A-F]{3}){1,2}$", std::regex_constants::icase);
    if (std::regex_match(string, validate_hex_code) == false) {printf("[!] Malformed hex-color: %s (replaced with magenta)\n", string.c_str()); return error_color;}

    // removes the # at the beginning of the string
    string.erase(0, 1);

    // if it's a three-digit code, duplicate the values to make it six-digit
    if (string.size() == 3) {
        for (int i = 0; i < 3; i++) {
            string.insert(i*2, 1, string[i*2]);
        }
    }

    // converts the hex digits into integers
    unsigned long hexval = stoul(string, nullptr, 16);

    color.r = (hexval >> 16) & 0xff;
    color.g = (hexval >> 8) & 0xff;
    color.b = (hexval >> 0) & 0xff;
    color.a = 255;

    return color;
}

SDL_Color get_color(int col = 0) {
    // returns a color, used in shapes and backgrounds

    if (col == 16) {return {0, 0, 0, 0};}
    if (col > 16 | col < 0) {return {255, 255, 255, 255};}

    return color_table[col];
}

void reset_color_table() {
    for (int i = 0; i < 16; i++) {
        color_table[i] = default_color_table[i];
    }

    return;
}

void set_color_table(int id, string hex_color) {
    // Sets value in the color table
    // ----------------------------------------------------------
    // id: color table index ID, ranges 0-15
    // rgb_top: top-color in RGBA

    if (id > 15 | id < 0) {return;}
    color_table[id] = hex_string_to_color(hex_color);
    return;
}

void set_combo_timer(int ms) {
    combo_display_timer = ms;
}

void draw_gradient(int x = 0, int y = 0, int w = width, int h = height, SDL_Color rgb_bottom = {255, 255, 255, 255}, SDL_Color rgb_top = {0, 0, 0, 255}) {
    // Gradient drawing function
    // ----------------------------------------------------------
    // x, y, w, h: gradient coords,     e.g. "0, 0, 320, 240"
    // rgb_top: top-color in RGBA
    // rgb_bottom: bottom-color in RGBA

    for (int i = 0; i < h; i++) {
        int r = ((rgb_bottom.r - rgb_top.r) * i)/h + rgb_top.r;
        int g = ((rgb_bottom.g - rgb_top.g) * i)/h + rgb_top.g;
        int b = ((rgb_bottom.b - rgb_top.b) * i)/h + rgb_top.b;
        int a = ((rgb_bottom.a - rgb_top.a) * i)/h + rgb_top.a;

        SDL_SetRenderDrawColor(renderer, r, g, b, a);

        SDL_RenderDrawLine(renderer, x, y + i, x+w, y + i);
    }

    return;
}

void draw_text(string text, int x, int y, int scale = 1, int align = 1, int max_width = width, SDL_Color mul = {255, 255, 255}) {
    // Bitmap monospaced font-drawing function, supports printable ASCII only
    // ----------------------------------------------------------
    // text: a std string,          e.g. "Hello World"
    // x, y: x and y coordinates,   e.g. "320, 240"
    // scale: scaling factor,       e.g. "2" for double-size text
    // align: alignment setting     e.g. 0 for centered, 1 for right-align, -1 for left-align
    // max_width: max width that text can occupy; set to 0 to disable
    // mul: SDL_Color to multiply font texture with (in other words, the text color)

    // printable ASCII (use this string for making new fonts):
    //  !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~

    // skips the entire function if the font happens to have not loaded for whatever reason
    // prevents a crash
    if (font == NULL) {
        return;
    }

    SDL_SetTextureScaleMode(font_texture, SDL_ScaleModeNearest);
    SDL_SetTextureColorMod(font_texture, mul.r, mul.g, mul.b);
    SDL_Rect src;
    SDL_Rect dest;

    int char_width  = font->w/95;
    int char_height = font->h;
    int scaled_char_width = char_width;
    int text_size = text.size();

    // resizes characters if need be
    if (max_width < text_size * (char_width * scale) && max_width != 0) {
        scaled_char_width = max_width / text_size;
    } else {
        scaled_char_width *= scale;
    }

    for (int i = 0; i < text_size; ++i) {
        // get ASCII value of current character
        // we also define char properties here due to some resizing shenanagains later
        int char_value = text[i] - 32;
        int align_offset = 0;

        // get character coords in source image
        // width and height are 1 character
        src.x = (char_value * char_width);
        src.y = 0;
        src.w = char_width;
        src.h = char_height;

        // determine offset value to use
        if (align >= 1) {align_offset = 0;}
        else if (align == 0) {align_offset = ((text_size * scaled_char_width)/2) * -1;}
        else if (align <= -1) {align_offset = (text_size * scaled_char_width) * -1;}

        // get x and y coords, offset by current character count and align/scale factors
        // width and height bound-box get scaled here as well
        dest.x = x + (i * scaled_char_width) + align_offset;
        dest.y = y;
        dest.w = scaled_char_width;
        dest.h = char_height * scale;

        // skip character if it's out of view
        if (dest.x > width || dest.x < -dest.w || dest.y > height || dest.y < -dest.h) {continue;}

        SDL_RenderCopy(renderer, font_texture, &src, &dest);
    }
    return;
}

void draw_background_test(bg_data bg_data, int frame_time) {
    // Debugging background; shows visual bars of song_tick and beat_tick, a square being scaled on every beat, and displays beat count info

    SDL_Rect shape;
    static int peak_beat_length;
    static int last_beat_length;

    // resets these values on beat 0; i.e. when a level starts
    if (bg_data.beat_count == 0) {
        peak_beat_length = 0;
        last_beat_length = 0;
    }

    // these values count up the current beat length
    // useful for measuring margains of error with song ticking
    if (last_beat_length > bg_data.beat_tick) {
        peak_beat_length = last_beat_length;
    }

    last_beat_length = bg_data.beat_tick;

    // red bar that shows song playback
    shape.x = 0;
    shape.y = height - 32;
    shape.h = 32;
    shape.w = bg_data.song_tick * 0.01;

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &shape);
    draw_text(std::to_string(bg_data.song_tick), 0, shape.y, 1, 1);

    // green bar that shows beat length
    shape.y = height - 64;
    shape.w = bg_data.beat_tick;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderFillRect(renderer, &shape);
    draw_text(std::to_string(bg_data.beat_tick), 0, shape.y, 1, 1);

    // yellow bar that shows peak of last beat
    shape.y = height - 96;
    shape.w = peak_beat_length;

    SDL_SetRenderDrawColor(renderer, 255, 172, 0, 255);
    SDL_RenderFillRect(renderer, &shape);
    draw_text(std::to_string(peak_beat_length), 0, shape.y, 1, 1);

    // box that pulses on every beat, also shows the beat count
    // color shows timing window; red=none, green=left, blue=right
    int scale = fmax(width/22, (width/22)*2 - (bg_data.beat_tick/2));

    shape.x = width/8 - (scale/2);
    shape.y = height/2 - (width/22 / 2) - (scale/2);
    shape.w = width/22 + scale;
    shape.h = shape.w;

    switch (check_beat_timing_window()) {
        case 0:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;

        case 1:
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            break;

        case 2:
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            break;
    }

    SDL_RenderFillRect(renderer, &shape);
    draw_text(std::to_string(bg_data.beat_count), shape.x + (shape.w*0.5), shape.y + (shape.h*0.5), 2, 0);
    
    // shows sequence strings on-screen for debugging
    draw_text(get_cpu_sequence(), width/2, height - font->h, 1, 0, width, {128, 64, 64, 255});
    draw_text(get_player_sequence(), width/2, height - (font->h*2), 1, 0, width, {64, 64, 128, 255});

    return;
}

void draw_background_solid(bg_data bg_data, int frame_time) {
    SDL_Color darkened_color;
    darkened_color.r = fmax(bg_data.grid_color.r * 0.75, 0);
    darkened_color.g = fmax(bg_data.grid_color.g * 0.75, 0);
    darkened_color.b = fmax(bg_data.grid_color.b * 0.75, 0);

    SDL_SetRenderDrawColor(renderer, darkened_color.r, darkened_color.g, darkened_color.b, 255);
    SDL_RenderClear(renderer);
    return;
}

void draw_background_tile(bg_data bg_data, int frame_time) {
    int max_tile_count = 12;
    int greater_axis = fmax(width, height);
    int scale_mul = fmax(floor(greater_axis/(aux_texture_h * max_tile_count)), 1);
    int tile_size = aux_texture_h * scale_mul;
    int tile_anim_size = fmax(floor(aux_texture_w / aux_texture_h), 1);
    int slow_song_tick = bg_data.song_tick * 0.0075;

    SDL_Rect tile;
    SDL_Rect tile_crop;

    tile.w = tile.h = tile_size;
    tile_crop = tile_frames[slow_song_tick % tile_frames.size()];

    for (int i = 0; i < width; i += tile_size) {
        for (int j = 0; j < height; j += tile_size) {
            tile.x = i;
            tile.y = j;
            SDL_RenderCopy(renderer, aux_texture, &tile_crop, &tile);
        }
    }

    return;
}

void draw_background_checkerboard(bg_data bg_data, int frame_time) {
    SDL_Rect shape;

    int square_size = fmax(width, height)/24;
    int slow_song_tick = bg_data.song_tick * (square_size * 0.00175);
    int scroll_speed = slow_song_tick % square_size;
    float beat_scale = fmax(0, (square_size*0.4 / (1 + bg_data.beat_tick*0.0125)));

    int first_beat_scale = 0;
    if (bg_data.beat_count%4 == 0 && bg_data.beat_count > bg_data.start_offset) {first_beat_scale = beat_scale;}

    int second_beat_scale = 0;
    if (bg_data.beat_count%4 == 2 && bg_data.beat_count > bg_data.start_offset) {second_beat_scale = beat_scale;}


    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (int i = square_size * -1; i < width; i += square_size) {
        for (int j = square_size; j < height + square_size; j += square_size) {
            // upper-left squares
            shape.x = (i + scroll_speed) - (first_beat_scale * 0.5);
            shape.y = (j - square_size) - (first_beat_scale * 0.5);
            shape.w = square_size * 0.5 + first_beat_scale;
            shape.h = square_size * 0.5 + first_beat_scale;

            SDL_SetRenderDrawColor(renderer, 64 + (first_beat_scale*8), 64, 64, 96 + (first_beat_scale*2));
            SDL_RenderFillRect(renderer, &shape);

            // lower-right squares
            shape.x = (i + scroll_speed) + (square_size * 0.5) - (second_beat_scale * 0.5);
            shape.y = (j - square_size) + (square_size * 0.5) - (second_beat_scale * 0.5);
            shape.w = square_size * 0.5 + second_beat_scale;
            shape.h = square_size * 0.5 + second_beat_scale;

            SDL_SetRenderDrawColor(renderer, 64, 64, 64 + (second_beat_scale*8), 96 + (second_beat_scale*2));
            SDL_RenderFillRect(renderer, &shape);
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    return;
}

void draw_background_fire(bg_data bg_data, int frame_time) {
    SDL_Rect shape;
    int square_size = (fmax(aux_texture_w, aux_texture_h) * 0.01) + 1;
    int square_size_pad = 0;
    int slow_song_tick = bg_data.song_tick * (square_size * 0.025);
    int direction_speed;
    int wave;

    SDL_SetRenderTarget(renderer, aux_texture);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // increases the size of each fire square when moving to the next shape
    // gives a cool "burst" effect to the fire
    if (bg_data.shape_advanced == true) aux_int = 1000;
    if (aux_int > 0) {
        aux_int = fmax(aux_int - frame_time, 0);
        square_size_pad = aux_int / 5.f;
    }
    
    // darken previous texture
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, frame_time);
    SDL_RenderFillRect(renderer, NULL);
    
    for (int i = -4; i < (fmax(aux_texture_w, aux_texture_h)/square_size) + 4; i++) {
        // alternates the direction of every other fire particle
        if (i%2 == 0) {
            direction_speed = 10 * -square_size;
            wave = cos(slow_song_tick / 360.f) * 100;
        } else {
            direction_speed = 10 * square_size;
            wave = sin(slow_song_tick / 360.f) * 100;
        }

        shape.y = aux_texture_h - (slow_song_tick + (i*7) * (i*11)) % aux_texture_h;
        shape.x = wave + (i * square_size) + ((shape.y * direction_speed) / aux_texture_h) - square_size_pad/2;
        shape.w = square_size_pad + square_size + (shape.y/4);
        shape.h = square_size_pad + square_size + (shape.y/4);

        int scaled_color = (shape.y * 255) / aux_texture_h;

        SDL_SetRenderDrawColor(renderer, 255, scaled_color, fmax(0, scaled_color - 80), scaled_color * 0.5);
        SDL_RenderFillRect(renderer, &shape);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, aux_texture, NULL, NULL);
    return;
}

void draw_background_conway(bg_data bg_data, int frame_time) {
    int greater_axis = fmax(width, height);

    SDL_Rect bg;
    bg.w = greater_axis;
    bg.h = greater_axis;

    if (greater_axis == width) {
        bg.x = 0;
        bg.y = (greater_axis - height) * -0.5;
    } else {
        bg.x = (greater_axis - width) * -0.5;
        bg.y = 0;
    }

    SDL_SetRenderTarget(renderer, aux_texture);
    bool temp_arr[32][32];

    // create a duplicate of current aux array, this is so the cells dont
    // homogenize into one big block from dynamically modifying them one at a time
    if (bg_data.beat_advanced) {
        for (int y = 0; y < 32; y++) {
            for (int x = 0; x < 32; x++) {
                temp_arr[x][y] = aux_bool_array[x][y];
            }
        }
    }

    // this for loop does the actual conway iteration
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            if (bg_data.beat_advanced) {
                // check surrounding area; the total number of surrounding pixels is then used to iterate conway
                int alive_neighbours = 0;

                for (int a = -1; a < 2; a++) {
                    for (int b = -1; b < 2; b++) {
                        int neighbour_x = (x + a);
                        int neighbour_y = (y + b);

                        // awkward workaround because -1 % 31 returns -1 rather than 31
                        if (neighbour_x < 0) {
                            neighbour_x += 32;
                        }

                        if (neighbour_y < 0) {
                            neighbour_y += 32;
                        }

                        // excludes offsets of 0,0
                        if (!(a == 0 && b == 0)) {
                            if (temp_arr[neighbour_x][neighbour_y]) {alive_neighbours++;}
                        }

                    }
                }

                // keep alive if there's 3 neighbours (or 2 and it was alive before), otherwise kill
                if (temp_arr[x][y] && alive_neighbours == 2 || alive_neighbours == 3) {
                    aux_bool_array[x][y] = true;
                } else {
                    aux_bool_array[x][y] = false;
                }
            }

            if (aux_bool_array[x][y]) {
                SDL_SetRenderDrawColor(renderer, 255, 160, 255, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }

            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, aux_texture, NULL, &bg);
    return;
}

void draw_background_monitor(bg_data bg_data, int frame_time) {
    SDL_Rect shape;
    int scanline_height = fmax(fmax(width, height) * 0.0025, 1);
    int slow_song_tick = bg_data.song_tick * (scanline_height * 0.0075);
    int scanline_yoffset = slow_song_tick % (scanline_height * 6);
    
    if (bg_data.shape_advanced) aux_int = 750;

    shape.x = 0;
    shape.w = width;
    shape.h = scanline_height;

    SDL_Color darkened_color;
    darkened_color.r = fmax(bg_data.grid_color.r * 0.25, 0);
    darkened_color.g = fmax(bg_data.grid_color.g * 0.25, 0);
    darkened_color.b = fmax(bg_data.grid_color.b * 0.25, 0);

    SDL_SetRenderDrawColor(renderer, darkened_color.r, darkened_color.g, darkened_color.b, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 32);
    
    if (aux_int > 0) {
        aux_int = fmax(aux_int - frame_time, 0);
        
        // generate and display noise texture
        void *pixels;
        int pitch;
        
        SDL_LockTexture(aux_texture, NULL, &pixels, &pitch);
        
        for (int y = 0; y < aux_texture_h; y++) {
            Uint32* dest = (Uint32*)((Uint8*)pixels + y * pitch);
            for (int x = 0; x < aux_texture_w; x++) {
                Uint32 shade = 0;
                Uint8 r, g, b, a;
                
                r = (Uint8)rand();
                g = (Uint8)rand();
                b = (Uint8)rand();
                a = (Uint8)rand()/4;
                
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                    shade += a;
                    shade += b << 8;
                    shade += g << 16;
                    shade += r << 24;
                } else {
                    shade += r;
                    shade += g << 8;
                    shade += b << 16;
                    shade += a << 24;
                }
                
                *dest++ = shade;
            }
        }
        
        SDL_UnlockTexture(aux_texture);
    
        SDL_SetTextureBlendMode(aux_texture, SDL_BLENDMODE_ADD);
        SDL_RenderCopy(renderer, aux_texture, NULL, NULL);
        SDL_SetTextureBlendMode(aux_texture, SDL_BLENDMODE_BLEND);
    }

    for (int y = scanline_height * -1; y < height; y += scanline_height * 6) {
        shape.y = y + scanline_yoffset;
        SDL_RenderFillRect(renderer, &shape);
    }
    return;
}

void draw_background_wave(bg_data bg_data, int frame_time) {
    float scroll_speed = bg_data.song_tick * 0.001;
    float wave_size = width / 2.f;
    int qtr_height = (height / 4);
    int half_height = (height / 2);

    SDL_Color darkened_color;
    darkened_color.r = fmax(bg_data.grid_color.r * 0.5, 0);
    darkened_color.g = fmax(bg_data.grid_color.g * 0.5, 0);
    darkened_color.b = fmax(bg_data.grid_color.b * 0.5, 0);

    SDL_SetRenderDrawColor(renderer, darkened_color.r, darkened_color.g, darkened_color.b, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, fmax(bg_data.grid_color.r, 32), fmax(bg_data.grid_color.g, 32), fmax(bg_data.grid_color.b, 32), 64);

    for (int i = 0; i < width; i++) {
        int y = sin(i/wave_size + scroll_speed) * qtr_height + half_height;
        SDL_RenderDrawLine(renderer, i, height, i, height - y);

        y = cos(i/wave_size + scroll_speed * 0.70) * qtr_height + (half_height * 0.75);
        SDL_RenderDrawLine(renderer, i, height, i, height - y);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    return;
}

void draw_background_starfield(bg_data bg_data, int frame_time) {
    int tick_rate = 16;
    
    // essentially locks the background's updating rate
    // prevents ludicrously fast scrolling/star spawning at higher FPSes
    aux_int += frame_time;
    
    if (aux_int >= tick_rate) {
        
        SDL_SetRenderTarget(renderer, aux_texture);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        
        // darken previous texture
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 16);
        SDL_RenderFillRect(renderer, NULL);
        
        SDL_Rect offset;
        offset.x = 0;
        offset.y = -1;
        offset.w = aux_texture_w;
        offset.h = aux_texture_h;
        
        for (int i = 0; i < 4; i++) {
            SDL_SetRenderDrawColor(renderer, (rand() % 2 * 255), (rand() % 2 * 255), (rand() % 2 * 255), 255);
            SDL_RenderDrawPoint(renderer, rand() % aux_texture_w, rand() % aux_texture_h-1);
        }
        
        SDL_RenderCopy(renderer, aux_texture, NULL, &offset);
    
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderTarget(renderer, NULL);
        aux_int -= tick_rate;
    }
    
    SDL_RenderCopy(renderer, aux_texture, NULL, NULL);
    return;
}

void draw_background_hexagon(bg_data bg_data, int frame_time) {
    float scroll_speed = frame_time / 1000.f;
    float hex_width = fmax(width, height);
    float center_x = width/2.f;
    float center_y = height/2.f;

    // calculates two colors from grid_color
    SDL_Color color_a;
    color_a.r = fmax(bg_data.grid_color.r * 0.6, 0);
    color_a.g = fmax(bg_data.grid_color.g * 0.6, 0);
    color_a.b = fmax(bg_data.grid_color.b * 0.6, 0);
    
    // color_b's subtraction is deliberately allowed to underflow for some funky color combos
    SDL_Color color_b;
    color_b.r = 128 - color_a.r;
    color_b.g = 128 - color_a.g;
    color_b.b = 128 - color_a.b;

    // changes direction of rotation every other measure
    if ((bg_data.beat_count % (bg_data.measure_length*4)) >= (bg_data.measure_length*2)) {
        scroll_speed = -scroll_speed;
    }

    // stops all rotation during the intro
    if (bg_data.beat_count <= bg_data.start_offset) {
        scroll_speed = 0;
    }
    
    aux_float += scroll_speed;

    // swaps colors every few beats
    if (bg_data.beat_count%4 < 2) {
        SDL_Color tmp = color_a;
        color_a = color_b;
        color_b = tmp;
    }

    // calculates the six points (and center vertex) of a hexagon
    float angles[6];
    for (char i = 0; i < 6; i++) {
        int angle_value = 60 * (i + 1);
        angles[i] = aux_float + angle_value * to_rad;
    }
    
    // put the calculated angles into vertexes, the float typecasts are there so MinGW doesn't throw up warnings
    SDL_Vertex vertex_center = {{center_x, center_y}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_topl = {{center_x + (hex_width * (float)cos(angles[0])), center_y + (hex_width * (float)sin(angles[0]))}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_topr = {{center_x + (hex_width * (float)cos(angles[1])), center_y + (hex_width * (float)sin(angles[1]))}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_midl = {{center_x + (hex_width * (float)cos(angles[2])), center_y + (hex_width * (float)sin(angles[2]))}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_midr = {{center_x + (hex_width * (float)cos(angles[5])), center_y + (hex_width * (float)sin(angles[5]))}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_botl = {{center_x + (hex_width * (float)cos(angles[3])), center_y + (hex_width * (float)sin(angles[3]))}, color_a, {0.f, 0.f}};
    SDL_Vertex vertex_botr = {{center_x + (hex_width * (float)cos(angles[4])), center_y + (hex_width * (float)sin(angles[4]))}, color_a, {0.f, 0.f}};

    // connects the vertexes as tris for rendering (SDL treats every three-pair of vertexes as a tri)
    // we only need to connect half of the slices of the hexagon, the background color fills in the rest
    SDL_Vertex hex_vertex[9] = {
        vertex_topl,
        vertex_topr,
        vertex_center,
        vertex_midl,
        vertex_botl,
        vertex_center,
        vertex_botr,
        vertex_midr,
        vertex_center
    };

    SDL_SetRenderDrawColor(renderer, color_b.r, color_b.g, color_b.b, 255);
    SDL_RenderClear(renderer);
    SDL_RenderGeometry(renderer, NULL, hex_vertex, 9, NULL, 0);

    return;
}

void draw_background_munching(bg_data bg_data, int frame_time) {
    void *pixels;
    int pitch;
    float munch_rate_r = sin(bg_data.song_tick/200.f);
    float munch_rate_g = sin(bg_data.song_tick/200.f);
    float munch_rate_b = sin(bg_data.song_tick/200.f);
    SDL_Rect tile;
    
    tile.x = tile.y = 0;
    tile.h = tile.w = aux_texture_w;
    
    if (bg_data.shape_advanced) aux_int = 2000;
    
    // offsets each RGB channel's "munch" rate
    if (aux_int > 0) {
        aux_int = fmax(aux_int - frame_time, 0);
        
        munch_rate_r = sin(bg_data.song_tick/250.f);
        munch_rate_g = sin(bg_data.song_tick/350.f);
        munch_rate_b = sin(bg_data.song_tick/150.f);
    }
    
    SDL_LockTexture(aux_texture, NULL, &pixels, &pitch);
    
    // generate the munch texture
    for (int y = 0; y < aux_texture_h; y++) {
        Uint32* dest = (Uint32*)((Uint8*)pixels + y * pitch);
        for (int x = 0; x < aux_texture_w; x++) {
            Uint32 shade = 0;
            Uint8 r, g, b;
            
            r = (Uint8)((x ^ y) + munch_rate_r * 128);
            g = (Uint8)((x ^ y) + munch_rate_g * 128);
            b = (Uint8)((x ^ y) + munch_rate_b * 128);
            
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                shade += 0xff;
                shade += b << 8;
                shade += g << 16;
                shade += r << 24;
            } else {
                shade += r;
                shade += g << 8;
                shade += b << 16;
                shade += 0xff << 24;
            }
            
            *dest++ = shade;
        }
    }
    
    SDL_UnlockTexture(aux_texture);
    
    // tile resulting texture to fill screen
    for (int i = 0; i < width; i += tile.w) {
        for (int j = 0; j < height; j += tile.h) {
            tile.x = i;
            tile.y = j;
            SDL_RenderCopy(renderer, aux_texture, NULL, &tile);
        }
    }

    return;
}

void draw_background_lasers(bg_data bg_data, int frame_time) {
    int background_mul = 4;
    int pivot_x1 = cos((bg_data.song_tick + aux_int + 200) * 0.00065) * aux_texture_w;
    int pivot_x2 = cos((bg_data.song_tick + aux_int + 400) * 0.00075) * aux_texture_w;
    int pivot_x3 = cos((bg_data.song_tick + aux_int + 600) * 0.00085) * aux_texture_w;
    int pivot_y1 = sin((bg_data.song_tick + aux_int + 200) * 0.00105) * aux_texture_h;
    int pivot_y2 = sin((bg_data.song_tick + aux_int + 400) * 0.00115) * aux_texture_h;
    int pivot_y3 = sin((bg_data.song_tick + aux_int + 600) * 0.00125) * aux_texture_h;
    
    if (bg_data.shape_advanced) {
        aux_float = 1000;
    }
    
    if (aux_float > 0) {
        background_mul = 2;
        aux_int += fmin(40, (aux_float/4));
        aux_float = fmax(aux_float - frame_time, 0);
    }
    
    SDL_SetRenderTarget(renderer, aux_texture);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    SDL_SetRenderDrawColor(renderer, 8, 32, 16, frame_time*background_mul);
    SDL_RenderFillRect(renderer, NULL);
    
    SDL_SetRenderDrawColor(renderer, 16, 255, 64, 255);
    
    SDL_RenderDrawLine(renderer, pivot_x1, pivot_y1, aux_texture_w, aux_texture_h);
    SDL_RenderDrawLine(renderer, pivot_x2, pivot_y2, aux_texture_w, aux_texture_h);
    SDL_RenderDrawLine(renderer, pivot_x3, pivot_y3, aux_texture_w, aux_texture_h);
    SDL_RenderDrawLine(renderer, aux_texture_w - pivot_x1, pivot_y1, 0, aux_texture_h);
    SDL_RenderDrawLine(renderer, aux_texture_w - pivot_x2, pivot_y2, 0, aux_texture_h);
    SDL_RenderDrawLine(renderer, aux_texture_w - pivot_x3, pivot_y3, 0, aux_texture_h);
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, aux_texture, NULL, NULL);
    return;
}

void init_background_effect(background_effect effect_id) {
    // Initialize function for background effects
    // Used for setting up things like auxillary textures
    // ----------------------------------------------------------
    // effect_id: numeric ID for function (see switch statement below)

    printf("Initializing background (internal ID: %d)...\n", effect_id);
    SDL_DestroyTexture(aux_texture);
    aux_texture_w = 0;
    aux_texture_h = 0;
    aux_int = 0;
    aux_float = 0;

    switch (effect_id) {
        case tile:
            load_background_tileset();
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            load_tile_frame_file();
            break;
            
        case fire:
        case lasers:
            aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, width, height);
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            break;
            
        case starfield:
            aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, width/4, height/4);
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            break;

        case conway:
            aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, 32, 32);
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            
            for (int y = 0; y < 32; y++) {
                for (int x = 0; x < 32; x++) {
                    bool randomized = rand() & 1;
                    aux_bool_array[x][y] = randomized;
                }
            }

            break;
        
        case monitor:
            aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 320, 240);
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            break;
            
        case munching:
            aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 256, 256);
            SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
            break;

        case none:
        default: break;
    }

    return;
}

void draw_background_effect(background_effect effect_id, bg_data bg_data, bool draw_debug_bg, int frame_time) {
    // Master function that calls various background FX drawing functions
    // ----------------------------------------------------------
    // bg_data: struct containing various values (see background.h; draw_game())
    // draw_debug_bg: toggles whether to draw the debug background; -d switch must be on for this to work
    
    // caps frame_time value to prevent weirdness with certain BGFX like fire
    // those BGFX use the frame_time value to calculate fade-out effects that are consistent
    // regardless of framerate; however FPS values past a certain point disable this fade
    // resulting in a glitchy background effect
    if (frame_time <= 2) {frame_time = 2;}

    switch (effect_id) {
        case solid:         draw_background_solid       (bg_data, frame_time);  break;
        case checkerboard:  draw_background_checkerboard(bg_data, frame_time);  break;
        case tile:          draw_background_tile        (bg_data, frame_time);  break;
        case fire:          draw_background_fire        (bg_data, frame_time);  break;
        case conway:        draw_background_conway      (bg_data, frame_time);  break;
        case monitor:       draw_background_monitor     (bg_data, frame_time);  break;
        case wave:          draw_background_wave        (bg_data, frame_time);  break;
        case starfield:     draw_background_starfield   (bg_data, frame_time);  break;
        case hexagon:       draw_background_hexagon     (bg_data, frame_time);  break;
        case munching:      draw_background_munching    (bg_data, frame_time);  break;
        case lasers:        draw_background_lasers      (bg_data, frame_time);  break;
        case none:
        default: break;
    }

    if (get_debug() && draw_debug_bg) {
        draw_background_test(bg_data, frame_time);
    }

    return;
}

void draw_grid(int x = width/2, int y = height/2, int scale = height/22, SDL_Color rgb = {255, 255, 255, 255}, bool background_only = false) {
    // Draws the grid that shapes are placed on
    // ----------------------------------------------------------
    // x, y: center of grid,        e.g. "320, 240"
    // scale: size of square in px, e.g. "48"
    // rgb: color of background
    // background_only: toggles whether to draw the gridlines or not

    SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, rgb.a);
    SDL_Rect gridbox;

    // draws grid background
    gridbox.x = x - (scale * 7.5) - (scale/3);
    gridbox.y = y - (scale * 7.5) - (scale/3);
    gridbox.w = (scale * 15) + ((scale/3)*2);
    gridbox.h = gridbox.w;

    SDL_RenderFillRect(renderer, &gridbox);
    SDL_SetRenderDrawColor(renderer, abs(rgb.r - 64), abs(rgb.g - 64), abs(rgb.b - 64), 255);

    // draws the actual grid
    if (!background_only) {
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                gridbox.x = (x + (scale * i)) - (scale * 7.5);
                gridbox.y = (y + (scale * j)) - (scale * 7.5);
                gridbox.w = scale;
                gridbox.h = scale;
                SDL_RenderDrawRect(renderer, &gridbox);
            }
        }
    }

    return;
}

void draw_hud(int life, int score, int time, int frame_time) {
    // Draws the HUD during the main game
    
    int scale_mul = fmax(floor(height/360), 1);
    string score_string = std::to_string(score);
    score_string.insert(0, 8 - score_string.length(), '0');
    
    // draws the underlay (the black part)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    
    SDL_Rect hud_bar, life_bar;
    hud_bar.x = hud_bar.y = 0;
    hud_bar.w = width;
    hud_bar.h = (font->h + 8) * scale_mul;
    
    life_bar.h = hud_bar.h * 0.8;
    life_bar.x = life_bar.y = hud_bar.h * 0.1;
    life_bar.w = (width / 4) - (hud_bar.h * 0.1);
    
    SDL_RenderFillRect(renderer, &hud_bar);
    
    // draws the life bar
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_RenderFillRect(renderer, &life_bar);
    
    life_bar.w = life * (life_bar.w/100.f);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &life_bar);
    
    // draws text versions of life bar, and score
    draw_text(std::to_string(life) + "%", life_bar.x, life_bar.y, scale_mul, 1, hud_bar.w);
    draw_text(score_string, width - life_bar.x, life_bar.y, scale_mul, -1, hud_bar.w);
    
    if (combo_display_timer > 0) {
        int combo = get_combo();
        string combo_str = std::to_string(combo) + "x combo!";
        
        Uint8 color_pulse = abs(sin(time*4.f/180)) * 200;
        draw_text(combo_str, width/2, life_bar.y, scale_mul, 0, hud_bar.w/2, {255, color_pulse, 255, 255});
        
        combo_display_timer -= frame_time;
    }
    
    return;
}

void draw_game_over(int time) {
    // Draws the game over screen
    int scale_mul = fmax(floor(height/360), 1);
    int font_height = font->h * scale_mul;
    
    // darken the entire screen
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 64);
    SDL_RenderFillRect(renderer, NULL);
    
    // color-cycle for game over text color
    Uint8 color_pulse = abs(sin(time*0.4/180)) * 200;
    
    draw_text("GAME OVER", width/2, height/2 - font_height, scale_mul * 2, 0, width, {255, color_pulse, 0, 255});
    draw_text("Press any button to return to the menu.", width/2, height/2 + font_height, scale_mul, 0, width);
    
    return;
}

void draw_shape(int type = 0, int x = 7, int y = 7, int scale = 1, SDL_Color rgb = {0, 0, 0, 255}, int gx = 0, int gy = 0, float gscale = height/22.f) {
    // Shape-drawing function, used to render every shape
    // ----------------------------------------------------------
    // shape: type of shape to draw (range: 0-2; -1 = no draw)
    // x, y: position on grid       (range: 0-14)
    // scale: size in grid squares  (range: 1-8)
    // rgb: color of shape in RGBA values
    // gx, gy: position of drawgrid (can be used as an offset)
    // gscale: size of 1 grid square in pixels
    
    SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, rgb.a);

    x = gscale * (x + 0.5) + gx;
    y = gscale * (y + 0.5) + gy;
    int size = gscale * (1 + 2 * (scale-1));
    SDL_Rect shape;

    switch (type) {
        // circle
        case 0: {
            int y1, y2;
            float r = gscale/2 * (1 + 2 * (scale-1));

            for (y1 = -r, y2 = r; y1; y1++, y2--) {
                int xr = (int)(sqrt(r*r - y1*y1) + 0.5);

                shape.x = x - xr;
                shape.y = y + y1;
                shape.w = 2 * xr;
                shape.h = 1;
                SDL_RenderFillRect(renderer, &shape);

                shape.y = y + y2;
                SDL_RenderFillRect(renderer, &shape);
            }

            shape.x = x - r;
            shape.y = y;
            shape.w = 2 * r;
            shape.h = 1;

            SDL_RenderFillRect(renderer, &shape);
            return;
        }

        // square
        case 1: {
            shape.x = x - (size * 0.5);
            shape.y = y - (size * 0.5);
            shape.w = shape.h = size;

            SDL_RenderFillRect(renderer, &shape);
            return;
        }

        // triangle
        case 2: {
            int y1 = y - (size * 0.5);
            int y2 = y1 + size;

            for (int y3 = 0; y1 < y2; y1++, y3++) {
                shape.x = (x - (size*0.25)) + ((y-y1) * 0.5);
                shape.y = y1;
                shape.w = y3;
                shape.h = 1;

                SDL_RenderFillRect(renderer, &shape);
            }
            return;
        }

        // none/null
        case -1:
        default:
            return;
    }

    return;
}

void draw_character(int beat_count = 0) {
    // Draws character graphics on both sides of the main grid
    // ----------------------------------------------------------
    // beat_count: current beat value (used for calculating idle frame #)

    SDL_Rect char_coords, char_crop;
    int scale = width/8;

    // skip rendering if char_texture is NULL (this happens if the texture can't be loaded for whatever reason)
    if (char_texture == NULL) {
        return;
    }

    char_coords.x = width/8 - (scale/2);
    char_coords.y = height/2 - (width/22 / 2) - (scale/2);
    char_coords.w = width/22 + scale;
    char_coords.h = char_coords.w;

    char_crop = get_character_rect(beat_count);
    SDL_RenderCopy(renderer, char_texture, &char_crop, &char_coords);

    // moves from the left side to right side
    char_coords.x = width - (width/8 - (scale/2) + char_coords.w);
    SDL_RenderCopy(renderer, char_texture, &char_crop, &char_coords);

    return;
}

void draw_menu_background(int frame_time) {
    // Draws a sine-wave array of shapes
    // Used in menus as a decoration

    int time = SDL_GetTicks() / 32;
    float shape_size = fmax(width, height)/16.f;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < 16; i++) {
        int x = (i * shape_size);
        int y = height/2 + (sin((time + (i*64)) * to_rad) * shape_size);

        draw_shape(i%3, 0, 0, 1, {255,255,255,64}, x, y, shape_size);
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    return;
}

void draw_fps(bool toggle, int fps, int frame_time) {
    if (toggle) {
        string fps_string = std::to_string(fps).append(" FPS");
        string frame_time_string = std::to_string(frame_time).append(" ms");

        // draws a black, transparent rectangle underneath the FPS text
        SDL_Rect rect;

        rect.w = (font->w/95) * 8;
        rect.h = font->h * 2;
        rect.x = 0;
        rect.y = 0;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // does the actual FPS text rendering
        draw_text(fps_string, 0, 0, 1, 1);
        draw_text(frame_time_string, 0, font->h, 1, 1);
    }
    return;
}

void draw_fade(int fadein_mul, int fadeout_mul, int frame_time) {
    // Draws both the fade-in and fade-out effects
    // Called every frame whenever transitioning between screens
    
    if (fade_in == 0 && fade_out == 255) {return;}

    if (fade_in >= 0) {fade_in -= (fadein_mul * 0.0625) * frame_time;}
    if (fade_out > 0 && fade_in <= 0) {fade_out += (fadeout_mul * 0.0625) * frame_time;}

    // clamps both of these values to 0-255
    fade_in = fmin(fmax(fade_in, 0), 255);
    fade_out = fmin(fmax(fade_out, 0), 255);

    // gets whatever fade value is higher
    int fade_priority = fmax(fade_in, fade_out);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, fade_priority);

    // draw to entire screen (hence the NULL)
    // see https://wiki.libsdl.org/SDL_RenderFillRect
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    return;
}

void draw_level_intro_fade(int song_start_time, int current_time, int intro_beat_length) {
    // Draws fade-in during intro, tied to song intro progression
    // ----------------------------------------------------------
    // song_start_time: timestamp of song starting
    // current_time: current timestamp (via SDL_GetTicks)
    // intro_beat_length: time in milliseconds that the intro will last

    int half_intro = intro_beat_length * 0.5;

    if (song_start_time + half_intro <= current_time) {return;}

    // the first half of this equation calculates how many ms are left in the intro
    // the second half clamps it to a 255-to-0 range
    int opacity = (half_intro - (current_time - song_start_time)) * (255.f / half_intro);

    // extra clamp, just in case!
    opacity = fmin(fmax(opacity, 0), 255);

    // sets up fade opacity
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, opacity);

    // draw to entire screen (hence the NULL)
    // see https://wiki.libsdl.org/SDL_RenderFillRect
    SDL_RenderFillRect(renderer, NULL);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    return;
}

void draw_loading(bool fill_black = false) {
    // Draws loading text and banner
    // ----------------------------------------------------------
    // fill_black: Toggle to fill screen with all-black

    if (fill_black) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    } else {
        SDL_Rect rect;
        int font_height = font->h;

        rect.x = 0;
        rect.y = height/2 - font_height;
        rect.w = width;
        rect.h = font_height * 3;

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    draw_text("Loading...", width/2, height/2, 1, 0);

    // note this use of RenderPresent; this IMMEDIATELY renders the loading text
    SDL_RenderPresent(renderer);

    return;
}

bool draw_warning(int frame_time) {
    // Grays out background
    SDL_SetRenderDrawColor(renderer, 64, 64, 72, 255);
    SDL_RenderClear(renderer);

    // used primarily in drawtext calls
    int scale_mul = fmax(floor(height/360), 1);

    // Draws the eye icon
    draw_shape(0, 5, 3, 3, {255, 255, 255, 255},    width/2 - height/22 * 7.5, height/2 - height/22 * 7.5);
    draw_shape(0, 9, 3, 3, {255, 255, 255, 255},    width/2 - height/22 * 7.5, height/2 - height/22 * 7.5);
    draw_shape(1, 7, 3, 3, {255, 255, 255, 255},    width/2 - height/22 * 7.5, height/2 - height/22 * 7.5);
    draw_shape(0, 7, 3, 2, {64, 64, 72, 255},       width/2 - height/22 * 7.5, height/2 - height/22 * 7.5);
    draw_shape(2, 7, 3, 1, {255, 255, 255, 255},    width/2 - height/22 * 7.5, height/2 - height/22 * 7.5);

    // Draws all the warning text
    draw_text("PHOTOSENSITIVITY WARNING", width/2, height/12, scale_mul + 1, 0);
    draw_text("This game contains bright colors and rapidly-flashing lights.",        width/2, height/2 + (20*scale_mul), 1, 0);
    draw_text("These effects can trigger seizures in a small percentage of people.",    width/2, height/2 + (40*scale_mul), 1, 0);
    draw_text("If you or your relatives have a history of photo-sensitive epilepsy,",   width/2, height/2 + (60*scale_mul), 1, 0);
    draw_text("then do not play this game without first consulting a physician.",       width/2, height/2 + (80*scale_mul), 1, 0);

    // Calculates and draws fade on text
    if (fade_in == 0) {
        static float text_fade = 255;

        // this is defined in order to avoid a compiler warning
        int enter_text_startcoord = height/1.25;
        SDL_Rect enterText = {0, enter_text_startcoord, width, (scale_mul+1)*40};

        draw_text("Press Start to continue.", width/2, height/1.25, scale_mul+1, 0);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 64, 64, 72, text_fade);
        SDL_RenderFillRect(renderer, &enterText);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        if (text_fade >= 0) {text_fade -= 0.25 * frame_time;}
        if (text_fade <= 0) {text_fade = 0;}
    }

    draw_fade(4, 4, frame_time);

    return true;
}

bool draw_title(int menu_selection, int frame_time) {
    // Draws the title screen
    // ----------------------------------------------------------
    // menu_selection: What is currently selected (range 0-3)
    
    const char *menu_items[4] = {"Play", "Sandbox", "Options", "Quit"};

    int scale_mul = fmax(floor(fmin(height, width)/360), 1);
    int char_height = font->h + 2;

    SDL_SetRenderDrawColor(renderer, 235, 130, 0, 255);
    SDL_RenderClear(renderer);
    draw_menu_background(frame_time);

    SDL_Rect rect;

    // renders the logo (if applicable)
    if (logo_texture != NULL) {
        int w, h;
        SDL_QueryTexture(logo_texture, NULL, NULL, &w, &h);

        rect.w = width/1.5;
        rect.h = rect.w / ((float)w / (float)h);
        rect.x = width/2 - rect.w/2;
        rect.y = height/3 - rect.h/2;

        SDL_RenderCopy(renderer, logo_texture, NULL, &rect);
    }

    // draws MOTD text
    draw_text(get_motd(), width/2, rect.y + rect.h + font->h, 1, 0, width);

    // draws dark underlay to emphasize selected item
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);

    rect.x = width * 0.25;
    rect.y = height/1.5 + ((menu_selection * char_height) * scale_mul);
    rect.w = width * 0.5;
    rect.h = char_height * scale_mul;

    SDL_RenderFillRect(renderer, &rect);

    // draws menu items
    for (int i = 0; i < 4; i++) {
        SDL_Color text_highlight = {255, 255, 255};

        if (i == menu_selection) {text_highlight = {255, 255, 96};}

        draw_text(menu_items[i], width/2, height/1.5 + ((i * char_height) * scale_mul), scale_mul, 0, width, text_highlight);
    }
    
    // draws version number
    draw_text(get_version_string(), 0, height - font->h, 1, 1, width, {0, 0, 32});
    
    if (get_debug()) {
        draw_text("debug mode is enabled! goofy things may happen", width, height - font->h, 1, -1, width, {0, 0, 32});
    }

    draw_fade(8, 16, frame_time);
    return true;
}

bool draw_options(int frame_time) {
    // Draws the options menu
    // ----------------------------------------------------------
    // See options.cpp for more info!
    
    int option_selection = get_option_selection();
    int scale_mul = fmax(floor(fmin(height, width)/360), 1);
    int char_height = font->h + 2;
    int left_edge = width/8;
    int right_edge = width - (left_edge * 2);
    SDL_Rect rect;

    rect.h = char_height * scale_mul;

    draw_gradient(0, 0, width, height, {96, 255, 128});
    draw_menu_background(frame_time);

    // draws dark underlay to emphasize selected item
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    rect.x = left_edge - 8;
    rect.y = height/8 + ((option_selection * char_height) * scale_mul);
    rect.w = right_edge + 8;
    SDL_RenderFillRect(renderer, &rect);

    // draws option items
    for (int i = 0; i < option_count; i++) {
        SDL_Color text_highlight = {255, 255, 255};
        string option_value = "";

        // draw the labels
        if (i == option_selection) {text_highlight = {255, 255, 96};}
        draw_text(get_option_name(i), left_edge, height/8 + ((i * char_height) * scale_mul), scale_mul, 1, width/2, text_highlight);

        // get a string for the option value (if there is one)
        option_value = get_option_value(i);
        
        // colorize "Enabled" and "Disabled" strings
        if (option_value == "Enabled") {text_highlight = {96, 255, 96};}
        if (option_value == "Disabled") {text_highlight = {255, 96, 96};}

        // draw the value of the option
        draw_text(option_value, left_edge + right_edge - 8, height/8 + ((char_height*i) * scale_mul), scale_mul, -1, width/4, text_highlight);
    }

    // draws menu descriptions
    rect.x = 0;
    rect.y = height - (char_height * scale_mul);
    rect.w = width;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    SDL_RenderFillRect(renderer, &rect);
    draw_text(get_option_desc(), width*0.01, height - (char_height * scale_mul), scale_mul, 1, width);

    draw_fade(16, 16, frame_time);

    return true;
}

bool draw_level_select(json json_file, int frame_time) {
    // Draws the level select menu
    // ----------------------------------------------------------
    // json_file: The currently-loaded level's JSON object
    
    int scale_mul = fmax(floor(fmin(height, width)/300), 1);

    // calculates the size of the shape grid; this will be used to create a texture that our shapes are drawn on later
    SDL_Rect grid_area;
    grid_area.x = width/2 - (height/22 * 7.5);
    grid_area.y =  height/2 - (height/22 * 7.5);
    grid_area.w =  (height/22) * 15;
    grid_area.h =  grid_area.w;

    // json_file is set to NULL if it cannot be parsed properly
    // if it returns NULL, we draw an error screen instead
    if (json_file == NULL) {
        draw_gradient(0, 0, width, height, {255, 32, 96});
        draw_grid(width/2, height/2, height/22, {0, 0, 0, 255}, true);

        int time = SDL_GetTicks();
        int color_pulse = sin(time*0.4/180) * 120;

        // draws a ! mark
        // note the use of Uint8 here is due to SDL_Color's specifications
        Uint8 blue_component = fmax(color_pulse, 0);
        draw_shape(2, 7, 7,  8, {255, 255, blue_component, 255},    grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(0, 7, 12, 1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 10, 1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 9,  1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 8,  1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 7,  1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 6,  1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);
        draw_shape(1, 7, 5,  1, {0, 0, 0, 255},     grid_area.x, grid_area.y, grid_area.w/15);

        draw_text("An error has occurred while trying to load a level.", width/2, grid_area.y + grid_area.h + 16, scale_mul, 0, width, {255, 192, 32});
        draw_text("Check the console or log file for details.", width/2, grid_area.y + grid_area.h + (font->h * scale_mul) + 16, scale_mul, 0);
    } else {
        draw_gradient(0, 0, width, height, {0, 0, 255});
        draw_grid(width/2, height/2, height/22, get_color(get_bg_color()), true);

        // all shapes are rendered to a texture in order to allow for "erase" colors to function
        SDL_Texture *shape_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, grid_area.w, grid_area.w);
        SDL_SetRenderTarget(renderer, shape_texture);

        // clear out any garbage data that the texture might have
        // fixes a very nasty feedback loop bug in "real" fullscreen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        for (int i = 1; i < json_file.size(); i++) {
            draw_shape(
                json_file[i].value("shape", 0),
                json_file[i].value("x", 7),
                json_file[i].value("y", 7),
                json_file[i].value("scale", 1),
                get_color(json_file[i].value("color", 0)),
                0, 0, grid_area.w/15);

            // check if auto_shapes are present, and if so, draw them
            if (json_file[i].contains("auto_shapes") && json_file[i]["auto_shapes"].is_array()) {
                for (int j = 0; j < json_file[i]["auto_shapes"].size(); j++) {
                    draw_shape(
                        json_file[i]["auto_shapes"][j].value("shape", 0),
                        json_file[i]["auto_shapes"][j].value("x", 7),
                        json_file[i]["auto_shapes"][j].value("y", 7),
                        json_file[i]["auto_shapes"][j].value("scale", 1),
                        get_color(json_file[i]["auto_shapes"][j].value("color", 0)),
                        0, 0, grid_area.w/15);
                }
            }
        }

        SDL_SetTextureBlendMode(shape_texture, SDL_BLENDMODE_BLEND);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, shape_texture, NULL, &grid_area);
        SDL_DestroyTexture(shape_texture);

        draw_text(get_level_name(), width/2, grid_area.y - (font->h*(2+scale_mul)), scale_mul, 0);
        draw_text(std::to_string(get_level_bpm()) + " BPM", width/6, grid_area.y + grid_area.h + (font->h), 1, 1, width/3);
        draw_text("Genre: " + get_genre(), width/6, grid_area.y + grid_area.h + (font->h * 2), 1, 1, width/3);
        draw_text("Song: " + get_song_author(), width - (width/6), grid_area.y + grid_area.h + (font->h), 1, -1, width/3);
        draw_text("Level: " + get_level_author(), width - (width/6), grid_area.y + grid_area.h + (font->h * 2), 1, -1, width/3);
        
        if (get_debug()) {
            for (int i = 0; i < 16; i++) {
                SDL_Rect color_box;
                color_box.w = color_box.h = 8;
                color_box.x = 0;
                color_box.y = i * color_box.h;
                
                SDL_Color color = get_color(i);
                
                SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
                SDL_RenderFillRect(renderer, &color_box);
            }
        }
    }

    draw_fade(16, 16, frame_time);
    return true;
}

bool draw_game(int beat_count, int start_offset, int measure_length, int song_start_time, float beat_start_time, int current_ticks, int intro_beat_length, bool beat_advanced, bool shape_advanced, background_effect background_id, shape active_shape, shape result_shape, std::vector<shape> previous_shapes, bool grid_toggle, bool song_over, bool game_over, int frame_time) {
    // Main function used during gameplay
    // ----------------------------------------------------------
    // TODO: the # of arguments here could be heavily reduced with "get_foobar"-style functions
    // See background.h for bg_data parameters
    // active_shape: The shape the player is controlling
    // result_shape: The shape the CPU is controlling
    // previous_shapes: Array of all previously-placed shapes
    // grid_toggle: Toggles whether to draw the grid pattern
    // song_over: True when the last shape has been successfully placed
    // game_over: True when the player hits 0% health
    
    SDL_RenderClear(renderer);
    SDL_Color bg_color = get_color(get_bg_color());

    int character_beat_count = ((beat_count - start_offset) <= 0) ? 0: (beat_count - (start_offset + 1));
    
    // sets up bg_data
    bg_data bg_data = {
        (current_ticks - song_start_time), 
        (int)(current_ticks - beat_start_time),
        beat_advanced,
        shape_advanced,
        beat_count - 1,
        start_offset - 1,
        measure_length,
        bg_color
    };

    // the order here is deliberately chosen to give the best clarity
    // regardless of game resolution; the grid should ALWAYS be visible
    // even if it's at the cost of everything else
    draw_background_effect(background_id, bg_data, true, frame_time);
    draw_character(character_beat_count);
    draw_grid(width/2, height/2, height/22, bg_color, !grid_toggle);

    SDL_Rect grid_area;
    grid_area.x = width/2 - (height/22 * 7.5);
    grid_area.y = height/2 - (height/22 * 7.5);
    grid_area.w = grid_area.h = (height/22) * 15;

    // all shapes are rendered to a texture in order to allow for "erase" colors to function
    SDL_Texture *shape_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, grid_area.w, grid_area.w);
    SDL_SetRenderTarget(renderer, shape_texture);

    // clear out any garbage data that the texture might have
    // fixes a nasty feedback loop bug in "real" fullscreen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // draws every previous shape
    for (int i = 0; i < previous_shapes.size(); i++) {
        draw_shape(
            previous_shapes[i].type,
            previous_shapes[i].x,
            previous_shapes[i].y,
            previous_shapes[i].scale,
            get_color(previous_shapes[i].color),
            0, 0, grid_area.w/15);
    }

    if (song_over == false && game_over == false && beat_count > start_offset) {
        // this draws the CPU's shape
        if ((beat_count - 1 - start_offset)%(measure_length*2) < measure_length) {
            Uint8 color_r = abs(sin(current_ticks/160.f) * 255);
            Uint8 color_g = abs(sin(current_ticks/180.f) * 255);
            Uint8 color_b = abs(sin(current_ticks/200.f) * 255);

            draw_shape(
                result_shape.type,
                result_shape.x,
                result_shape.y,
                result_shape.scale,
                {color_r, color_g, color_b, 255},
                0, 0, grid_area.w/15);
        }

        // this draws the player's shape
        if ((beat_count - 1 - start_offset)%(measure_length*2) >= measure_length) {
            Uint8 color_r = abs(sin(current_ticks/160.f) * 255);
            Uint8 color_g = abs(sin(current_ticks/180.f) * 255);
            Uint8 color_b = abs(sin(current_ticks/200.f) * 255);

            draw_shape(
                active_shape.type,
                active_shape.x,
                active_shape.y,
                active_shape.scale,
                {color_r, color_g, color_b, 255},
                0, 0, grid_area.w/15);
        }
    }

    // draws the shapes onto the screen
    SDL_SetTextureBlendMode(shape_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, shape_texture, NULL, &grid_area);
    SDL_DestroyTexture(shape_texture);
    
    // draws the HUD elements
    if (game_over) {draw_game_over(current_ticks);}
    draw_hud(get_life(), get_score(), current_ticks, frame_time);
    
    draw_level_intro_fade(song_start_time, current_ticks, intro_beat_length);
    draw_fade(255, 8, frame_time);
    return true;
}

bool draw_sandbox(background_effect background_id, shape active_shape, std::vector<shape> previous_shapes, bool menu_open, int menu_item, int frame_time) {
    // Draws the screen during Sandbox mode
    // ----------------------------------------------------------
    // active_shape: The shape the player is currently controlling
    // previous_shapes: Array of all previously-placed shapes
    // menu_open: Toggles whether or not the toolbar is visible
    // menu_item: The currently-selected toolbar item
    
    int scale_mul = fmax(floor(fmin(height, width)/360), 1);
    int time = SDL_GetTicks();

    SDL_RenderClear(renderer);
    
    // sets up a dummy bgdata
    bg_data bg_data = {
        time, 
        0, 
        false,
        false,
        0, 
        0, 
        0, 
        get_color(15)
    };

    draw_background_effect(background_id, bg_data, false, frame_time);
    draw_grid(width/2, height/2, height/22, get_color(15));

    SDL_Rect grid_area;
    grid_area.x = width/2 - (height/22 * 7.5);
    grid_area.y =  height/2 - (height/22 * 7.5);
    grid_area.w =  (height/22) * 15;
    grid_area.h =  grid_area.w;

    // all shapes are rendered to a texture in order to allow for "erase" colors to function
    SDL_Texture *shape_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, grid_area.w, grid_area.w);
    SDL_SetRenderTarget(renderer, shape_texture);

    // clear out any garbage data that the texture might have
    // fixes a very nasty feedback loop bug in "real" fullscreen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (int i = 0; i < previous_shapes.size(); i++) {
        draw_shape(
            previous_shapes[i].type,
            previous_shapes[i].x,
            previous_shapes[i].y,
            previous_shapes[i].scale,
            get_color(previous_shapes[i].color),
            0, 0, grid_area.w/15);
    }

    draw_shape(
        active_shape.type,
        active_shape.x,
        active_shape.y,
        active_shape.scale,
        get_color(active_shape.color),
        0, 0, grid_area.w/15);

    SDL_SetTextureBlendMode(shape_texture, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, shape_texture, NULL, &grid_area);
    SDL_DestroyTexture(shape_texture);
    
    // draws the sandbox menu (if it's open)
    if (menu_open) {
        SDL_Rect icon_area;
        SDL_Rect icon_coords;
        int icon_tex_size;
        int icon_size = height/8;
        int icon_padding = icon_size / 10;
        
        SDL_QueryTexture(sandbox_icon_texture, NULL, NULL, NULL, &icon_tex_size);
        int total_width_of_icons = sandbox_item_count * (icon_size + icon_padding) - icon_padding;
        
        // draws the sandbox icons and boxes
        for (int i = 0; i < sandbox_item_count; i++) {
            Uint8 shade = 96;
            if (i == menu_item) {shade = (abs(sin(time*0.4/90)) * 30) + 220;}
            
            icon_area.x = (i * (icon_size + icon_padding)) + (width/2 - total_width_of_icons/2);
            icon_area.y = height - icon_size - (icon_padding/2);
            icon_area.w = icon_size;
            icon_area.h = icon_size;
            
            icon_coords.x = i * icon_tex_size;
            icon_coords.y = 0;
            icon_coords.w = icon_coords.h = icon_tex_size;
            
            SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
            SDL_RenderFillRect(renderer, &icon_area);
            
            icon_area.x += icon_padding;
            icon_area.y += icon_padding;
            icon_area.w = icon_area.h = icon_area.w - icon_padding * 2;
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderFillRect(renderer, &icon_area);
            
            icon_area.x += icon_padding;
            icon_area.y += icon_padding;
            icon_area.w = icon_area.h = icon_area.w - icon_padding * 2;
            
            SDL_RenderCopy(renderer, sandbox_icon_texture, &icon_coords, &icon_area);
        }
        
        draw_text(sandbox_items[menu_item], width/2, height - icon_size - icon_padding - font->h, 1, 0);
    }

    draw_fade(16, 16, frame_time);
    return true;
}