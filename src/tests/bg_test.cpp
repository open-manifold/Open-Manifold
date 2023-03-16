// This file is to be compiled on its own in order to test new background effects.
// This is separate from the main game; as such, it is not to be included in the list of source files when building.

#include <cstdlib>
#include <cmath>
#include <string>
#include <cstring>

#include <SDL2/SDL.h>

#include "../background.h"

// Declare global variables
SDL_Window* window;
SDL_Renderer* renderer;

int width  = 1280;
int height = 720;

int aux_int = 0;
float aux_float;
SDL_Texture* aux_texture;
int aux_texture_w;
int aux_texture_h;

void test_background_effect(bg_data bg_data, int frame_time) {

// Edit this function! This is the background effect to test.
// Note that bg_data parameters are NOT provided as of yet
    return;
}

void test_background_effect_init() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    //aux_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, width, height);
    //SDL_QueryTexture(aux_texture, NULL, NULL, &aux_texture_w, &aux_texture_h);
    return;
}

bool init(int argc, char *argv[]) {
    // initialize SDL stuff (video, audio, inputs, events, etc.)
    printf("Build date: %s at %s\n========================================\n"
    "There's no docs for this, so if something breaks you're on your own!\n", __DATE__, __TIME__);
    
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        printf("[!] SDL could not initialize! %s\n", SDL_GetError());
        return false;
    }
    
    // create window
    window = SDL_CreateWindow("Background Test Program", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
    
    if (window == NULL) {
        printf("[!] Error while creating window: %s\n", SDL_GetError()); 
        return false;
    }
    
    // create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);
    
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

SDL_Color get_color(int col = 0) {
    switch(col) {
        case 0:         return {255, 255, 255, 255};
        case 1:         return {0, 0, 255, 255};
        case 2:         return {0, 255, 0, 255};
        case 3:         return {0, 192, 192, 255};
        case 4:         return {255, 0, 0, 255};
        case 5:         return {255, 0, 255, 255};
        case 6:         return {255, 128, 0, 255};
        case 7:         return {192, 192, 192, 255};
        case 8:         return {128, 128, 128, 255};
        case 9:         return {128, 128, 255, 255};
        case 10:        return {128, 255, 128, 255};
        case 11:        return {128, 255, 255, 255};
        case 12:        return {255, 128, 128, 255};
        case 13:        return {255, 128, 255, 255};
        case 14:        return {255, 255, 16, 255};
        case 15:        return {0, 0, 0, 255};
        case 16:        return {0, 0, 0, 0};
        default:        return {255, 255, 255, 255};
    }
}

int main(int argc, char *argv[]) {
    
    if (!init(argc, argv)) return 1;
    
    // benchmark mode, invoke with -b argument
    // this is useful for benchmarking without requiring lower-end hardware to do direct profiling on
    if (argc == 2 && strncmp(argv[1], "-b", 2) == 0) {
        printf("Benchmark mode invoked. Re-creating renderer to remove V-sync...\n");
        
        // recreate renderer with no vsync of any kind
        SDL_DestroyRenderer(renderer);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
        
        int b_start_time = SDL_GetTicks();
        printf("Initializing BGFX...\n");
        test_background_effect_init();
        printf("Running 6,000 frames...\n");
        for (int i = 0; i < 6000; i++) {
            bg_data bg_data = {
                (int)SDL_GetTicks(),
                (int)SDL_GetTicks(),
                false,
                false,
                0,
                8,
                16,
                get_color(0)
            };
            
            test_background_effect(bg_data, 0);
            SDL_RenderPresent(renderer);
        }
        
        int b_end_time = SDL_GetTicks();
        
        printf("Benchmark time: %i milliseconds\n", b_end_time - b_start_time);
        printf("%fx real-time (assuming 60fps for 100s = realtime)\n", 100000.f / (float)(b_end_time - b_start_time));
        kill();
        return 0;
    }
    
    SDL_Event evt;
    
    // stores values for the FPS counter
    static int start_time, frame_time = 0;
    int fps = 0;
    int time_passed = 0;
    int frame_count = 0;
    int beat_count = 0;
    
    int bg_color = 0;
    SDL_Color color = get_color(bg_color);
    test_background_effect_init();

    bool program_running = true;

    // main loop that runs while the game is active
    while (program_running) {
        start_time = SDL_GetTicks();
        bool shape_advanced = false;
        
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
                
                // handles key presses
                case SDL_KEYDOWN:
                    switch (evt.key.keysym.sym) {
                        case SDLK_z:
                            bg_color = (bg_color + 1) % 16;
                            color = get_color(bg_color);
                            break;
                            
                        case SDLK_x:
                            bg_color = (bg_color + 16 - 1) % 16;
                            color = get_color(bg_color);
                            break;
                            
                        case SDLK_SPACE:
                            shape_advanced = true;
                            break;
                            
                        default:
                            break;
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        // calculate background data stuff
        // some of these are just filled in with default values
        bg_data bg_data = {
            (int)SDL_GetTicks(),
            time_passed,
            false,
            shape_advanced,
            beat_count,
            8,
            16,
            color
        };
        
        test_background_effect(bg_data, frame_time);
        SDL_RenderPresent(renderer);
        
        // calculates FPS
        frame_time = SDL_GetTicks() - start_time;
        
        frame_count++;
        time_passed += frame_time;
        
        if (time_passed >= 500) {
            beat_count++;
            fps = frame_count;
            frame_count = 0;
            time_passed = 0;
        }
    }

    kill();
    return 0;
}