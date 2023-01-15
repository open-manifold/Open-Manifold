#ifndef character
#define character

struct character_frames {
    std::vector<SDL_Rect> idle;
    SDL_Rect up;
    SDL_Rect down;
    SDL_Rect left;
    SDL_Rect right;
    SDL_Rect circle;
    SDL_Rect square;
    SDL_Rect triangle;
    SDL_Rect xplode;
    SDL_Rect scale_up;
    SDL_Rect scale_down;
};

void parse_character_file(nlohmann::json);
void set_character_frame_data(character_frames);
void set_character_status(char);
void reset_character_status();
void set_character_timer(int);
void tick_character(int);
SDL_Rect get_character_rect(int);

#endif