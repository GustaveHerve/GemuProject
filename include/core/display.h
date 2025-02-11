#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H

struct gb_core;
struct pixel;

void *get_frame_buffer(void);

void draw_pixel(struct gb_core *gb, struct pixel p);

void lcd_off(struct gb_core *gb);

#endif
