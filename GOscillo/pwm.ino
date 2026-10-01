#include "driver/ledc.h"
#include "hal/ledc_hal.h"
#include "hal/ledc_ll.h"

#ifdef ESP32_C3
#define GPIO_PIN 5  // should not assign GPIO #36 throough #39
#define LEDC_BIT_MAX 14
#define LEDC_PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_CHANNEL LEDC_CHANNEL_5
#define DIV_MAX 1023
#else
#define GPIO_PIN 16  // should not assign GPIO #36 throough #39
#define LEDC_BIT_MAX 17
#define LEDC_PWM_MODE LEDC_HIGH_SPEED_MODE
#define PWM_CHANNEL LEDC_CHANNEL_7
#define DIV_MAX 611
#endif
#define PWM_TIMER LEDC_TIMER_3

byte duty = 128;              // duty ratio = duty/256
byte p_range = LEDC_BIT_MAX;  // bit_num 1 - LEDC_BIT_MAX
unsigned short count;         // rate 1/1 - 1/1023
ledc_hal_context_t ledc_hal;

float pulse_frq(void) {  // 1Hz <= pulse_frq <= 40MHz
  float freq = 80.0e6 / (1 << p_range) / count;
  if (freq < 1.0) freq = 1.0;
  return freq;
}

#ifndef NOWEB
void set_pulse_frq(float freq) {  // 1Hz <= freq <= 40MHz
  if (freq > 40e6) freq = 40e6;
#ifdef ESP32_C3
  if (freq < 4.773) freq = 4.773;
#endif
  p_range = constrain(int(log(80e6 / freq) / log(2) - 9), 1, LEDC_BIT_MAX);
  count = round((80000000UL / (1 << p_range)) / freq);
  // Serial.print("freq: "); Serial.print(freq);
  // Serial.print(" range: "); Serial.print(p_range);
  // Serial.print(" count: "); Serial.println(count);
  pwm_timer_config();
  pwm_channel_config();
}
#endif

void pulse_init() {
  p_range = constrain(p_range, 1, LEDC_BIT_MAX);
  if (p_range < 2)
    count = constrain(count, 1, 1023);
  else
    count = constrain(count, 512, 1023);
  pwm_timer_config();
  pwm_channel_config();
}

void pwm_timer_config(void) {
  uint32_t fmax = 80000000 / (1 << p_range);
  uint32_t fmin = fmax / 1023 + 1;
  ledc_timer_config_t timer_conf = {
    .speed_mode = LEDC_PWM_MODE,
    .duty_resolution = (ledc_timer_bit_t)p_range,
    .timer_num = PWM_TIMER,
    .freq_hz = constrain((uint32_t)(pulse_frq()), fmin, fmax),
    .clk_cfg = LEDC_USE_APB_CLK
  };
  vTaskDelay(10);  // to avoid Guru Meditation Error
  ledc_timer_config(&timer_conf);
  vTaskDelay(10);  // to avoid Guru Meditation Error
}

void pwm_channel_config(void) {
  ledc_channel_config_t ch_conf = {
    .gpio_num = GPIO_PIN,
    .speed_mode = LEDC_PWM_MODE,
    .channel = PWM_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = PWM_TIMER,
    .duty = calc_duty(),
    .hpoint = 0
  };
  vTaskDelay(10);  // to avoid Guru Meditation Error
  ledc_channel_config(&ch_conf);
  ledc_hal_init(&ledc_hal, LEDC_PWM_MODE);
  uint32_t div_q10_8 = count << 8;
  ledc_hal_set_clock_divider(&ledc_hal, PWM_TIMER, div_q10_8);
  vTaskDelay(10);  // to avoid Guru Meditation Error
}

void update_frq(int diff) {
  int fast;
  long newCount;
  bool new_range = false;

  if (abs(diff) > 3) {
    fast = 8;
  } else if (abs(diff) > 2) {
    fast = 4;
  } else if (abs(diff) > 1) {
    fast = 2;
  } else {
    fast = 1;
  }
  newCount = (long)count + fast * diff;

  if (newCount > 1023) {
    if (p_range < LEDC_BIT_MAX) {
      ++p_range;
      newCount = 512;
      new_range = true;
    }
  } else if (newCount < 512) {
    if (p_range < 2) {
      newCount = constrain(newCount, 1, 511);
    } else {
      --p_range;
      newCount = 1023;
      new_range = true;
    }
  } else if (p_range >= LEDC_BIT_MAX) {
    newCount = constrain(newCount, 512, DIV_MAX); // lower limit 1Hz
  }
  count = newCount;
  // Serial.print("range: "); Serial.print(p_range);
  // Serial.print(" count: "); Serial.print(count);
  // Serial.print(" duty: "); Serial.println(calc_duty());
  ledc_ll_set_duty_resolution(ledc_hal.dev, LEDC_PWM_MODE, PWM_TIMER, p_range);
  ledc_hal_set_clock_divider(&ledc_hal, PWM_TIMER, count << 8);
#ifdef ESP32_C3
  ledc_ll_ls_timer_update(ledc_hal.dev, LEDC_PWM_MODE, PWM_TIMER);
#endif
  if (new_range) {
    ledc_ll_set_duty_int_part(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL, calc_duty());
    ledc_ll_set_duty_start(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL);
#ifdef ESP32_C3
    ledc_ll_ls_channel_update(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL);
#endif
  }
}

#ifndef NOLCD
void disp_pulse_frq(void) {
  float freq = pulse_frq();
  if (freq < 10.0) {
    display.print(freq, 5);
  } else if (freq < 100.0) {
    display.print(freq, 4);
  } else if (freq < 1000.0) {
    display.print(freq, 3);
  } else if (freq < 10000.0) {
    display.print(freq, 2);
  } else if (freq < 100000.0) {
    display.print(freq, 1);
  } else if (freq < 1000000.0) {
    display.print(freq * 1e-3, 2);
    display.print('k');
  } else if (freq < 10000000.0) {
    display.print(freq * 1e-6, 4);
    display.print('M');
  } else {
    display.print(freq * 1e-6, 3);
    display.print('M');
  }
  display.print("Hz ");
}

void disp_pulse_dty(void) {
  if (duty < 26)  // < 10.0%
    display.print(' ');
  display.print(duty * 100.0 / 256.0, 1);
  display.print("%");
}
#endif

void pulse_start(void) {
  pulse_init();
}

void pulse_close(void) {
  // ledc_set_duty(LEDC_PWM_MODE, PWM_CHANNEL, 0);
  // ledc_update_duty(LEDC_PWM_MODE, PWM_CHANNEL);
  // ledc_timer_pause(LEDC_PWM_MODE, PWM_TIMER);
  // ledc_stop(LEDC_PWM_MODE, PWM_CHANNEL, 1);
  pinMode(GPIO_PIN, INPUT_PULLUP);
}

uint32_t calc_duty(void) {
  uint32_t iduty;
  if (p_range == 1)
    iduty = 1;  // duty=0 will not work
  else
    iduty = ((long)duty << p_range) >> 8;
  return iduty;
}

void setduty(void) {
  ledc_ll_set_duty_int_part(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL, calc_duty());
  ledc_ll_set_duty_start(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL);
#ifdef ESP32_C3
  ledc_ll_ls_channel_update(ledc_hal.dev, LEDC_PWM_MODE, PWM_CHANNEL);
#endif
}
