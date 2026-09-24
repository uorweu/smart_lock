#include "lcd.h"
#include "gpio.h"
#include "driver/i2c.h"
#include <unistd.h>

#define LCD_ADDR 0x27
#define I2C_MASTER_NUM 0 
#define I2C_MASTER_FREQ_HZ 100000

void lcd_send_nibble(uint8_t nibble, uint8_t is_data){
  uint8_t data_shifted = (nibble << 4);
  data_shifted = data_shifted | 0x08;
  if (is_data == 1) {
    data_shifted = data_shifted | 0x01;
  }
  uint8_t pulse_high = data_shifted | 0x04;
  i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &pulse_high, 1, 1000/portTICK_PERIOD_MS);
  usleep(1000); 

  uint8_t pulse_low = data_shifted & ~0x04;
  i2c_master_write_to_device(I2C_MASTER_NUM, LCD_ADDR, &pulse_low, 1, 1000/portTICK_PERIOD_MS);

  usleep(1000);
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

