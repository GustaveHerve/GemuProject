#ifndef CORE_DISPLAY_H
#define CORE_DISPLAY_H

#include <stdint.h>

struct gb_core;
struct pixel;

#define COLOR_FROM_HEX(HEX) {((HEX) >> 16) & 0xFF, ((HEX) >> 8) & 0xFF, (HEX) & 0xFF}
#define HEX_FROM_COLOR(COL) ((COL).r << 16 | (COL).g << 8 | (COL).b)

struct color
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

void *get_frame_buffer(void);

void draw_pixel(struct gb_core *gb, struct pixel p);

void lcd_off(struct gb_core *gb);

void reset_palette(void);

struct color get_color_index(unsigned int index);

void set_color_index(struct color new_color, unsigned int index);

#endif
