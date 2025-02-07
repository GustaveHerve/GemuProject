#ifndef DISPLAY_H
#define DISPLAY_H

struct cpu;
struct pixel;

void *get_frame_buffer(void);

void draw_pixel(struct cpu *cpu, struct pixel p);

void lcd_off(struct cpu *cpu);

#endif
