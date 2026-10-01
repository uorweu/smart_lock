#ifndef LCD_H
#define LCD_H
#include <stdint.h>
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t);
void lcd_put_character(char c);
void lcd_put_string(const char *str);
void lcd_send_cmd(uint8_t cmd);
void lcd_send_nibble(uint8_t nibble, uint8_t is_data);
void update_lcd_ui(int pin_length, int attempts_left, int status);
void lcd_backlight_on(void);
void lcd_backlight_off(void);
#endif
