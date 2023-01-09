#ifndef background
#define background

// contains all the background effect IDs
// strings are converted to this and used internally
enum background_effect {
    none,
    solid,
    tile,
    fire,
    checkerboard,
    conway,
    monitor,
    wave,
	starfield,
	hexagon
};

// stores data that can be used by the background
// song_tick: how long the song has been playing in MS
// beat_tick: how long the current beat has been set for (this resets to 0 after every beat)
// beat_advanced: bool flag for whether or not the beat has advanced
// beat_count: current beat number (including intro beats, in other words start_offset)
// start_offset: how many beats before the song starts (default: 32)
// measure_length: how many beats in a measure (default: 16)
// grid_color: the background color of the grid
struct bg_data {
    int song_tick;
    int beat_tick;
    bool beat_advanced;
    int beat_count;
    int start_offset;
    int measure_length;
    SDL_Color grid_color;
};

#endif