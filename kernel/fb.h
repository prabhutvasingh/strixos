#ifndef FB_H
#define FB_H
#include <stdint.h>
#include <stddef.h>

// RGB framebuffer - VBE 1024x768x32 + VGA text fallback
// Backend credit in comments (no frontend spam)

void fb_init(void);
void fb_set_tty_active(int v);
int fb_is_tty_active(void);
int fb_is_graphics(void);
int fb_get_width(void);
int fb_get_height(void);
void fb_clear(uint32_t rgb);
void fb_put_pixel(int x, int y, uint32_t rgb);
void fb_fill_rect(int x, int y, int w, int h, uint32_t rgb);
void fb_draw_test(void);
void fb_info(void);
void fb_putchar(char c);
uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);

// ANSI true-color RGB for text mode (serial)
void rgb_print(const char* text, uint8_t r, uint8_t g, uint8_t b);
void rgb_print_bg(const char* text, uint8_t r, uint8_t g, uint8_t b, uint8_t br, uint8_t bg, uint8_t bb);

#endif
