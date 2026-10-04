#include "lcd.h"
#include "gpio.h"
#include "driver/i2c.h"
#include <unistd.h>

#define LCD_ADDR 0x27
#define I2C_MASTER_NUM 0 
#define I2C_MASTER_FREQ_HZ 400000 // Boosted I2C to 400kHz Fast Mode!

uint8_t lcd_backlight_val = 0x08;

void lcd_send_nibble(uint8_t nibble, uint8_t is_data){
  uint8_t data_shifted = (nibble << 4);
  data_shifted = data_shifted | lcd_backlight_val;
  if (is_data == 1) {
    data_shifted = data_shifted | 0x01;
  }
  uint8_t pulse_high = data_shifted | 0x04;
  i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &pulse_high, 1, 1000/portTICK_PERIOD_MS);
  
  // LCD enable pulse only needs to be >450ns. 10 microseconds is plenty!
  usleep(10); 

  uint8_t pulse_low = data_shifted & ~0x04;
  i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &pulse_low, 1, 1000/portTICK_PERIOD_MS);

  // Typical LCD command execution time is ~37 microseconds. 100us is perfectly safe!
  usleep(100);
}

void lcd_send_cmd(uint8_t cmd){
  uint8_t top_half = (cmd >> 4);
  uint8_t bottom_half = (cmd & 0x0F);
  lcd_send_nibble(top_half, 0);
  lcd_send_nibble(bottom_half, 0);
}

void lcd_put_character(char c){
  uint8_t top_half = (c >>4);
  uint8_t bottom_half = (c & 0x0F);
  lcd_send_nibble(top_half, 1);
  lcd_send_nibble(bottom_half, 1);
}

void lcd_clear(void) {
  lcd_send_cmd(0x01);
  usleep(2000);
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
  int row_offsets[] = {0x00, 0x40,0x14,0x54};
  lcd_send_cmd(0x80 | (col + row_offsets[row]));
}
void lcd_put_string(const char *str){
  while(*str){
    lcd_put_character(*str);
    str++;
  }
}

void lcd_init(void) {
  i2c_config_t config = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = I2C_SDA_PIN,
    .scl_io_num = I2C_SCL_PIN,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };
  i2c_param_config(I2C_MASTER_NUM, &config);
  i2c_driver_install(I2C_MASTER_NUM, config.mode, 0, 0, 0);
  usleep(50000);
  lcd_send_nibble(0x03, 0);
  usleep(5000);
  lcd_send_nibble(0x03, 0);
  usleep(5000);
  lcd_send_nibble(0x03, 0);
  lcd_send_nibble(0x02, 0);

  lcd_send_cmd(0x28);
  lcd_send_cmd(0x08);
  lcd_send_cmd(0x01);
  usleep(2000);
  lcd_send_cmd(0x06);
  lcd_send_cmd(0x0C);

}
#define UI_STATUS_TYPING   0
#define UI_STATUS_ACCEPTED 1
#define UI_STATUS_DENIED   2

void update_lcd_ui(int pin_length, int attempts_left, int status) {
  char buffer[21]; // Buffer to hold our formatted 20-character strings

  // --- ROW 0: The Header ---
  lcd_set_cursor(0, 0);
  lcd_put_string("ENTER PIN TO UNLOCK!");

  // --- ROW 1: The Stars ---
  lcd_set_cursor(0, 1);
  lcd_put_string("PIN: ");
  for (int i = 0; i < 10; i++) { // Assuming max 10 digit pin
    if (i < pin_length) {
      lcd_put_character('*');
    } else {
      lcd_put_character(' '); // Clear old stars
    }
  }

  // --- ROW 2: The Status ---
  lcd_set_cursor(0, 2);
  if (status == UI_STATUS_TYPING) {
    lcd_put_string("STATUS: TYPING...   ");
  } else if (status == UI_STATUS_ACCEPTED) {
    lcd_put_string("STATUS: ACCEPTED!   ");
  } else if (status == UI_STATUS_DENIED) {
    lcd_put_string("STATUS: DENIED!     ");
  }

  // --- ROW 3: Attempts Left ---
  lcd_set_cursor(0, 3);
  // Use sprintf to inject the integer into the string
  sprintf(buffer, "ATTEMPTS LEFT: %d   ", attempts_left);
  lcd_put_string(buffer);
}


void lcd_backlight_on(void) {
    lcd_backlight_val = 0x08;
    lcd_send_cmd(0);
}

void lcd_backlight_off(void) {
    lcd_backlight_val = 0x00;
    lcd_send_cmd(0); 
}
