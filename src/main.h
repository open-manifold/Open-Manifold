#pragma once

std::string get_level_background_effect_string();
std::string get_background_tile_path();
std::string get_character_tile_path();
std::string get_level_name();
std::string get_level_playlist_name();
std::string get_genre();
std::string get_level_author();
std::string get_song_author();
std::string get_motd();
std::string get_cpu_sequence();
std::string get_player_sequence();
std::string get_version_string();
std::string get_current_mapping();
std::string get_current_mapping_explicit(bool, unsigned int);
std::string get_input_name();
std::string get_lang_string(std::string);

void load_language(std::string);
void save_settings();
void set_music_volume();
void set_sfx_volume();
void set_fullscreen();
void set_channel_mix();
void play_channel_test();
void play_dialog_blip();
void play_dialog_advance();
void set_vsync_renderer();
void set_frame_cap_ms();
void init_controller();
void rumble_controller(int);
int get_controller_count();
void reset_controller_binds();
void reset_keyboard_binds();

int get_level_bpm();
int get_bg_color();
int check_beat_timing_window(unsigned int);
bool check_json_validity();
bool get_debug();

int get_life();
int get_score();
int get_combo();

int get_hiscore();
int get_play_count();
bool get_cleared();

void load_tile_frame_file();