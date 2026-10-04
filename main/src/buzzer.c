#include "buzzer.h"
#include "gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- PWM SETTINGS ---
#define BUZZER_TIMER       LEDC_TIMER_0
#define BUZZER_MODE        LEDC_LOW_SPEED_MODE
#define BUZZER_CHANNEL     LEDC_CHANNEL_0
#define BUZZER_DUTY_RES    LEDC_TIMER_13_BIT 
#define BUZZER_VOLUME      4096              

void buzzer_init(void) {
  ledc_timer_config_t ledc_timer = {
    .speed_mode       = BUZZER_MODE,
    .timer_num        = BUZZER_TIMER,
    .duty_resolution  = BUZZER_DUTY_RES,
    .freq_hz          = 1000,  
    .clk_cfg          = LEDC_AUTO_CLK
  };
  ledc_timer_config(&ledc_timer);
  ledc_channel_config_t ledc_channel = {
    .speed_mode     = BUZZER_MODE,
    .channel        = BUZZER_CHANNEL,
    .timer_sel      = BUZZER_TIMER,
    .intr_type      = LEDC_INTR_DISABLE,
    .gpio_num       = BUZZER_PIN, 
    .duty           = 0,          
    .hpoint         = 0
  };
  ledc_channel_config(&ledc_channel);
}
void buzzer_play_tone(int frequency, int duration_ms) {
  // 1. Change the frequency to match the musical note
  ledc_set_freq(BUZZER_MODE, BUZZER_TIMER, frequency);

  // 2. Turn the volume up! (Set duty cycle to our BUZZER_VOLUME macro)
  ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, BUZZER_VOLUME);
  ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);

  // 3. Keep the buzzer on for the requested duration
  vTaskDelay(duration_ms / portTICK_PERIOD_MS);

  // 4. Turn the volume back to 0 when the note is finished
  buzzer_stop();

  // 5. Add a tiny 20ms pause between notes so they don't slur together
  vTaskDelay(20 / portTICK_PERIOD_MS);
}

  void buzzer_stop(void) {
    // Set the duty cycle (volume) to 0
    ledc_set_duty(BUZZER_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(BUZZER_MODE, BUZZER_CHANNEL);
  }


void melody_key_press(void) {
    buzzer_play_tone(NOTE_C6, 30);
}

void melody_success(void) {
    buzzer_play_tone(NOTE_C5, 150);
    buzzer_play_tone(NOTE_E5, 150);
    buzzer_play_tone(NOTE_G5, 150);
    buzzer_play_tone(NOTE_C6, 300);
}
void melody_error(void) {
    buzzer_play_tone(NOTE_G4, 150);
    buzzer_play_tone(NOTE_G4, 150);
}

void melody_locked_out(void) {
    buzzer_play_tone(NOTE_C5, 250);
    buzzer_play_tone(NOTE_G4, 250);
    buzzer_play_tone(NOTE_E4, 250);
    buzzer_play_tone(NOTE_C4, 600);
}

void melody_alarm(void) {
    buzzer_play_tone(NOTE_A5, 400);
    buzzer_play_tone(NOTE_F5, 400);
}
