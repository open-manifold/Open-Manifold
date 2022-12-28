// This file is to be compiled on its own in order to test character animation files.
// This is separate from the main game; as such, it is not to be included in the list of source files when building.

#include <cstdlib>
#include <cmath>
#include <string>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Declare global variables
SDL_Window* window;
SDL_Renderer* renderer;

int width  = 854;
int height = 480;

int bpm = 120;

SDL_Surface* font;
SDL_Texture* character_texture_map;

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

void load_font() {
    font = IMG_Load("assets/font.png");
    
    if (font == NULL) {
        printf("[!] Couldn't find font image! %s\n", SDL_GetError());
        font = SDL_CreateRGBSurface(0, 1, 1, 4, 0, 0, 0, 0);
    }
    
    return;
}

void draw_text(std::string text, int x, int y, int scale = 1, int align = 1, int max_width = width, SDL_Color mul = {255, 255, 255}) {
    // Bitmap font-drawing function, supports printable ASCII only
    // ----------------------------------------------------------
    // text: a std string,          e.g. "Hello World"
    // x, y: x and y coordinates,   e.g. "320, 240"
    // scale: scaling factor,       e.g. "2" for double-size text
    // align: alignment setting     e.g. 0 for centered, 1 for right-align, -1 for left-align
    // max_width: max width that text can occupy; set to 0 to disable

    // printable ASCII (use this string for making new fonts):
    //  !\"#$%'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
    
    // skips the entire function if the font happens to have not loaded for whatever reason
    // prevents a crash
    if (font == NULL) {
        return;
    }
    
    // enables bilinear filtering if text needs to be resized
    if (max_width < text.size() * ((font->w/95) * scale) && max_width != 0) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, font);
    SDL_SetTextureColorMod(texture, mul.r, mul.g, mul.b);
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
        if (align >= 1 ) {align_offset = 0;}
        else if (align == 0) {align_offset = ((text_size * scaled_char_width)/2) * -1;}
        else if (align <= -1 ) {align_offset = (text_size * scaled_char_width) * -1;}

        // get x and y coords, offset by current character count and align/scale factors
        // width and height bound-box get scaled here as well
        dest.x = x + (i * scaled_char_width) + align_offset;
        dest.y = y;
        dest.w = scaled_char_width;
        dest.h = char_height * scale;
        
        // skip character if it's out of view
        if (dest.x > width || dest.x < -dest.w || dest.y > height || dest.y < -dest.h) {continue;}

        SDL_RenderCopy(renderer, texture, &src, &dest);
    }

    SDL_DestroyTexture(texture);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    return;
}

void draw_fps(bool toggle, int fps) {
    if (toggle) {
        draw_text(std::to_string(fps).append(" FPS"), 0, 0, 1, 1);
    }
    return;
}

bool init(int argc, char *argv[]) {
    // initialize SDL stuff (video, audio, inputs, events, etc.)
    printf("Build date: %s at %s\n========================================\n"
	"To use this program: place a character.png and character.json in the assets folder\n"
	"and then run this program from the directory Open Manifold is stored in.\n"
	"There's no docs for this, so if something breaks you're on your own!\n", __DATE__, __TIME__);
    
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] SDL could not initialize! %s\n", SDL_GetError());
        return false;
    }
    
    // create window
    window = SDL_CreateWindow("Character Test Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
    
    if (window == NULL) {
        printf("[!] Error while creating window: %s\n", SDL_GetError()); 
        return false;
    }
    
    // create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (renderer == NULL) {
        printf("[!] Error while creating renderer: %s\n", SDL_GetError()); 
        return false;
    }
    
    load_font();
    
    return true;
}

void kill() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

character_frames parse_character_file(std::string file) {
    // loads a JSON character file and puts its image coordinate data into a struct
    // ----------------------------------------------------------
    // file: a path to a file ("assets/levels/foobar/character.json")
    
    std::ifstream ifs(file);
    nlohmann::json parsed_json;
	character_frames frame_data = {
		{{0, 0, 0, 0}},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0},
		{0, 0, 0, 0}
	};
    
    // checks to make sure the file exists
    if (std::filesystem::exists(file) == false) {
        printf("[!] Couldn't parse character file: Does not appear to exist.\n");
        return frame_data;
    }
    
    // checks to see if the JSON is valid JSON
    try {
        parsed_json = json::parse(ifs);
    } catch(json::parse_error& err) {
        printf("[!] Couldn't parse character file: %s\n", err.what());
        return frame_data;
    }
    
    // write the idle frame array
    std::vector<SDL_Rect> temp_vector;
	
	for (int i = 0; i < parsed_json[0]["frames"].size(); i++) {
		SDL_Rect temp_rect;
		int x = parsed_json[0]["frames"][i].value("x", 0);
		int y = parsed_json[0]["frames"][i].value("y", 0);
		int w = parsed_json[0]["frames"][i].value("w", 0);
		int h = parsed_json[0]["frames"][i].value("h", 0);
		
		temp_rect.x = parsed_json[0]["frames"][i].value("x", 0);
        temp_rect.y = parsed_json[0]["frames"][i].value("y", 0);
        temp_rect.w = parsed_json[0]["frames"][i].value("w", 0);
        temp_rect.h = parsed_json[0]["frames"][i].value("h", 0);
		
		temp_vector.push_back(temp_rect);
	}
	
	frame_data.idle = temp_vector;
    
    // write the other SDL_Rects; these are relatively simple as they're always gonna have 1 entry total
    for (int i = 1; i < 11; i++) {
        SDL_Rect temp_rect;
		
        temp_rect.x = parsed_json[i].value("x", 0);
        temp_rect.y = parsed_json[i].value("y", 0);
        temp_rect.w = parsed_json[i].value("w", 0);
        temp_rect.h = parsed_json[i].value("h", 0);
        
        switch (i) {
            case 1:  frame_data.up          = temp_rect; break;
            case 2:  frame_data.down        = temp_rect; break;
            case 3:  frame_data.left        = temp_rect; break;
            case 4:  frame_data.right       = temp_rect; break;
            case 5:  frame_data.circle      = temp_rect; break;
            case 6:  frame_data.square      = temp_rect; break;
            case 7:  frame_data.triangle    = temp_rect; break;
            case 8:  frame_data.xplode      = temp_rect; break;
            case 9:  frame_data.scale_up    = temp_rect; break;
            case 10: frame_data.scale_down  = temp_rect; break;
            default: break;
        }
    }
    
    return frame_data;
}

SDL_Rect get_character_rect(character_frames frames, int index) {
	switch (index) {
		case 0: return frames.idle[(SDL_GetTicks() / (60000 / (bpm*2)) % frames.idle.size())]; break;
		case 1: return frames.up; break;
		case 2: return frames.down; break;
		case 3: return frames.left; break;
		case 4: return frames.right; break;
		case 5: return frames.circle; break;
		case 6: return frames.square; break;
		case 7: return frames.triangle; break;
		case 8: return frames.xplode; break;
		case 9: return frames.scale_up; break;
		case 10: return frames.scale_down; break;
	}
	
	return {0, 0, 0, 0};
}

void draw_character_frame_name(int index) {
	std::string name = "";
	switch (index) {
		case 0: name = "Idle"; break;
		case 1: name = "Move Up"; break;
		case 2: name = "Move Left"; break;
		case 3: name = "Move Down"; break;
		case 4: name = "Move Right"; break;
		case 5: name = "Circle"; break;
		case 6: name = "Square"; break;
		case 7: name = "Triangle"; break;
		case 8: name = "X-Plode"; break;
		case 9: name = "Scale Up"; break;
		case 10: name = "Scale Down"; break;
	}
	
	draw_text(name, width/2, height/8, 1, 0);
    return;
}

int main(int argc, char *argv[]) {
    
    if (!init(argc, argv)) return 1;
    
    printf("Loading character test JSON file...\n");
    character_frames frame_data = parse_character_file("assets/character.json");
    
    printf("Loading character test image...\n");
    SDL_Surface* temp = IMG_Load("assets/character.png");
    character_texture_map = SDL_CreateTextureFromSurface(renderer, temp);
    
    SDL_Event evt;
    
    // stores values for the FPS counter
    static int start_time, frame_time = 0;
    int fps = 0;
    int time_passed = 0;
    int frame_count = 0;
	
	int character_index = 0;

    bool program_running = true;
	
	SDL_SetTextureScaleMode(character_texture_map, SDL_ScaleModeLinear);

    // main loop that runs while the game is active
    while (program_running) {
        start_time = SDL_GetTicks();
        
        // handles all events
        while (SDL_PollEvent(&evt) != 0) {
            switch (evt.type) {
            case SDL_QUIT:  program_running = false;   break;
            
            // handles resizing the window
            case SDL_WINDOWEVENT:
                if (evt.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_RenderClear(renderer);
                    SDL_GetWindowSize(window, &width, &height);
                }
                break;
			
			case SDL_KEYDOWN:
				switch(evt.key.keysym.sym) {
					case SDLK_LEFT:
                    character_index--;
					if (character_index < 0) {character_index = 10;}
                    break;
					
					case SDLK_RIGHT:
                    character_index++;
					if (character_index > 10) {character_index = 0;}
                    break;
					
					case SDLK_UP:
					bpm = fmin(bpm + 1, 300);
                    break;
					
					case SDLK_DOWN:
                    bpm = fmax(bpm - 1, 1);
                    break;
				}
				break;
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // render the lil guy poggies
        SDL_Rect char_texture_crop;
        SDL_Rect char_coords;
        
        int tex_w, tex_h;
        SDL_QueryTexture(character_texture_map, NULL, NULL, &tex_w, &tex_h);
        
        char_texture_crop = get_character_rect(frame_data, character_index);
        
        char_coords.x = width/2 - (char_texture_crop.w / 2) * 2;
        char_coords.y = height/2 - (char_texture_crop.h / 2) * 2;
        char_coords.w = char_texture_crop.w * 2;
        char_coords.h = char_texture_crop.h * 2;
        
        SDL_RenderCopy(renderer, character_texture_map, &char_texture_crop, &char_coords);
		draw_character_frame_name(character_index);
		draw_text(std::to_string(bpm), width/2, height/8 + font->h, 1, 0);
		draw_text("< and >: Select Animation", width/2, height - 64, 1, 0);
		draw_text("^ and V: Change Tempo", width/2, height - 64 - font->h, 1, 0);
        draw_fps(true, fps);
        
        SDL_RenderPresent(renderer);
        
        // calculates FPS
        frame_time = SDL_GetTicks() - start_time;
        
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