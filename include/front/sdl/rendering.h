#ifndef SDL_RENDERING_H
#define SDL_RENDERING_H

void set_window_title(const char *title);

int set_vsync(int val);

void render_frame_callback(void);

int init_rendering(void);

void free_rendering(void);

#endif
