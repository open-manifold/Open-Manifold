// This file is to be compiled on its own in order to test font fallback functionality.
// This is separate from the main game; as such, it is not to be included in the list of source files when building.

#include <cstdlib>
#include <string>

#include <SDL2/SDL.h>
#include "../font.h"

// Declare global variables
SDL_Window* window;
SDL_Renderer* renderer;

int width  = 854;
int height = 480;

SDL_Surface* font;

void load_font() {
    Uint32 rmask, gmask, bmask, amask;
    
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (fallback_font.bytes_per_pixel == 3) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = (fallback_font.bytes_per_pixel == 3) ? 0 : 0xff000000;
    #endif
    
    font = SDL_CreateRGBSurfaceFrom((void*)fallback_font.pixel_data, fallback_font.width, fallback_font.height, fallback_font.bytes_per_pixel*8, fallback_font.bytes_per_pixel*fallback_font.width, rmask, gmask, bmask, amask);
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

bool init(int argc, char *argv[]) {
    // initialize SDL stuff (video, audio, inputs, events, etc.)
    printf("Build date: %s at %s\n========================================\n"
	"There's no docs for this, so if something breaks you're on your own!\n", __DATE__, __TIME__);
    
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] SDL could not initialize! %s\n", SDL_GetError());
        return false;
    }
    
    // create window
    window = SDL_CreateWindow("Font Fallback Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
    
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
    
    return true;
}

void kill() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    if (!init(argc, argv)) return 1;
    SDL_Event evt;
    bool program_running = true;
    
    load_font();

    // main loop that runs while the game is active
    while (program_running) {
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
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
		draw_text("Hello, world!", width/2, height/2, 1, 0);
		draw_text("abcdefghijklmnopqrstuvwxyz", width/2, height/2 - font->h, 1, 0);
        draw_text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", width/2, height/2 - (font->h*2), 1, 0);
        draw_text("!@#$%^&*()_+-=<>:;/\\", width/2, height/2 - (font->h*3), 1, 0);
        
        SDL_RenderPresent(renderer);
    }

    kill();
    return 0;
}