#include "background.h"

#ifndef graphics
#define graphics

// dedicated struct for a shape, saves some time over using a JSON array
// doesn't contain sequence data
struct shape {
    int type;
    int x;
    int y;
    int scale;
    int color;
};

SDL_Color get_color(int);
void reset_color_table();
void set_color_table(int, std::string);
void set_combo_timer(int);

void draw_gradient(int, int, int, int, SDL_Color);
void draw_text(std::string, int, int, int, int, int, SDL_Color = {255, 255, 255});
void draw_grid(int, int, int, SDL_Color, bool);
void draw_shape(int, int, int, int, SDL_Color, int, int, float);
void draw_fps(bool, int, int);
void draw_fade(int, int, int);
void draw_level_intro_fade(int, int, int);
void load_logo();
void unload_logo();
void load_sandbox_icons();
void unload_sandbox_icons();
void load_font();

void init_background_effect(background_effect);
void draw_background_effect(background_effect, bg_data, bool, int);
void draw_menu_background(int);

void load_character_tileset();
void unload_character_tileset();
void draw_character(int);

void draw_loading(bool = false);
bool draw_warning(int);
bool draw_title(int, int);
bool draw_credits(int);
bool draw_level_select(nlohmann::json, int);
bool draw_game(int, int, int, int, float, int, int, bool, bool, background_effect, shape, shape, std::vector<shape>, bool, bool, bool, int);
bool draw_options(int, int, int, bool, int, bool, bool, bool, bool, bool, int, int);
bool draw_sandbox(background_effect, shape, std::vector<shape>, bool, int, int);

#endif