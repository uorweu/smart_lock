    #include <stdio.h>
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "driver/i2c.h"
    #include "driver/gpio.h"
    #include "driver/ledc.h"
    #include "rom/ets_sys.h"
    #include "GPIOs.h"

    #define LCD_ADDR 0x27
    #define I2C_NUM I2C_NUM_0

    void lcd_send_nibble(uint8_t nibble, uint8_t rs) {
        uint8_t data = (nibble << 4) | (rs ? 1 : 0) | 0x08;
        uint8_t buf[1];

        buf[0] = data | 0x04;
        i2c_master_write_to_device(I2C_NUM, LCD_ADDR, buf, 1, 1000 / portTICK_PERIOD_MS);
        ets_delay_us(1000);

        buf[0] = data & ~0x04;
        i2c_master_write_to_device(I2C_NUM, LCD_ADDR, buf, 1, 1000 / portTICK_PERIOD_MS);
        ets_delay_us(100);
    }

    void lcd_send_byte(uint8_t data, uint8_t rs) {
        lcd_send_nibble(data >> 4, rs);
        lcd_send_nibble(data & 0x0F, rs);
    }

    void lcd_init() {
        vTaskDelay(50 / portTICK_PERIOD_MS);
        lcd_send_nibble(0x03, 0);
        vTaskDelay(5 / portTICK_PERIOD_MS);
        lcd_send_nibble(0x03, 0);
        ets_delay_us(150);
        lcd_send_nibble(0x03, 0);

        lcd_send_nibble(0x02, 0);
        lcd_send_byte(0x28, 0);
        lcd_send_byte(0x08, 0);
        lcd_send_byte(0x01, 0);
        vTaskDelay(2 / portTICK_PERIOD_MS);
        lcd_send_byte(0x06, 0);
        lcd_send_byte(0x0C, 0);
    }

    void lcd_print(const char *str) {
        while (*str) {
            lcd_send_byte((uint8_t)(*str), 1);
            str++;
        }
    }

    void lcd_set_cursor(uint8_t col, uint8_t row) {
        uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
        lcd_send_byte(0x80 | (col + row_offsets[row]), 0);
    }

    void i2c_init() {
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_SDA_PIN,
            .scl_io_num = I2C_SCL_PIN,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 100000,
        };
        i2c_param_config(I2C_NUM, &conf);
        i2c_driver_install(I2C_NUM, conf.mode, 0, 0, 0);
    }

    void buzzer_init() {
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .timer_num        = LEDC_TIMER_0,
            .duty_resolution  = LEDC_TIMER_13_BIT,
            .freq_hz          = 2000,
            .clk_cfg          = LEDC_AUTO_CLK
        };
        ledc_timer_config(&ledc_timer);

        ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .timer_sel      = LEDC_TIMER_0,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = BUZZER_PIN,
            .duty           = 0,
            .hpoint         = 0
        };
        ledc_channel_config(&ledc_channel);
    }

    void buzzer_set(bool on) {
        if (on) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4096);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        } else {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        }
    }

    void pir_init() {
        gpio_reset_pin(PIR_PIN);
        gpio_set_direction(PIR_PIN, GPIO_MODE_INPUT);
        gpio_set_pull_mode(PIR_PIN, GPIO_PULLDOWN_ONLY);
    }

    void app_main(void) {
        i2c_init();
        lcd_init();
        buzzer_init();
        pir_init();

        lcd_set_cursor(0, 0);
        lcd_print("Sensor Test Mode");

        while (1) {
            int motion = gpio_get_level(PIR_PIN);

            if (motion) {
                lcd_set_cursor(0, 2);
                lcd_print("Motion: YES!    ");
                buzzer_set(true);
            } else {
                lcd_set_cursor(0, 2);
                lcd_print("Motion: None    ");
                buzzer_set(false);
            }

            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
