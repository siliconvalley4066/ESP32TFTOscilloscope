#if !defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI) && !defined(ARDUINO_ESP32C3_DEV) && !defined(ARDUINO_WAVESHARE_ESP32_C3_ZERO)
/*
   ESP32 Frequency Counter Library Version 1.03
   The max frequency is 40MHz at 80MHz APB clock.
   Stable and accurate pulse counting by hardware gating.
   Copyright (c) 2026, Siliconvalley4066
   Licenced under the GNU GPL Version 3.0
*/
#include "FreqCountESPgate.h"
#include "driver/ledc.h"
#include "hal/ledc_hal.h"

#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_TIMER   LEDC_TIMER_0
#define PWM_CHANNEL  LEDC_CHANNEL_7
#define PWM_TIMER    LEDC_TIMER_3
#define PWM_MODE     LEDC_HIGH_SPEED_MODE

volatile uint32_t FreqCountESPgate::count_ovf;
volatile uint32_t FreqCountESPgate::fcount;
volatile bool FreqCountESPgate::fflag;
bool FreqCountESPgate::first = true;
esp_timer_handle_t FreqCountESPgate::delay_int = NULL;
uint8_t FreqCountESPgate::gate_pin = TIME_GATE_PIN;

volatile uint16_t FreqCountESPgate::pulseCountx;
volatile uint32_t FreqCountESPgate::count_ovfx;

void IRAM_ATTR onPcnt(void *arg) {
  uint32_t status;
  if (pcnt_get_event_status(PCNT_UNIT, &status) != ESP_OK) return;
  PCNT.int_clr.val = BIT(PCNT_UNIT);  // Clear the interrupt
  PCNT.int_clr.val = BIT(PCNT_UNIT);  // Clear the interrupt again
  if (status & PCNT_EVT_H_LIM) {
    FreqCountESPgate::count_ovf++;  // high limit overflow
  }
  // if (status & PCNT_EVT_L_LIM) {
  //   count_ovf++;  // low limit overflow
  // }
}

void IRAM_ATTR onLedc() {
  // Start a one-shot timer to run after 100 us
  esp_timer_start_once(FreqCountESPgate::delay_int, 100); // 100us
}

void IRAM_ATTR onDelay(void * arg) {
  // This is called after 100 us onLedc()
  int16_t pulseCount;
  pcnt_get_counter_value(PCNT_UNIT, &pulseCount);
  FreqCountESPgate::pulseCountx = pulseCount;
  FreqCountESPgate::count_ovfx = FreqCountESPgate::count_ovf;
  FreqCountESPgate::fcount = (uint32_t)pulseCount + FreqCountESPgate::count_ovf * 32767;
  pcnt_counter_clear(PCNT_UNIT);
  FreqCountESPgate::count_ovf = 0;
  FreqCountESPgate::fflag = true;
}

void FreqCountESPgate::setupPcnt(uint8_t fpin, uint8_t gpin) {
  pcnt_config_t pcnt_config;
  pcnt_config.pulse_gpio_num = fpin;
  pcnt_config.ctrl_gpio_num = gpin;
  pcnt_config.unit = PCNT_UNIT;
  pcnt_config.channel = PCNT_CHANNEL_0;
  pcnt_config.pos_mode = PCNT_COUNT_INC;
  pcnt_config.neg_mode = PCNT_COUNT_DIS;
  pcnt_config.lctrl_mode = PCNT_MODE_DISABLE;
  pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.counter_h_lim = 32767;
  pcnt_config.counter_l_lim = -32768;

  // initialize PCNT unit
  pcnt_unit_config(&pcnt_config);

  // noise filter
  // pcnt_set_filter_value(PCNT_UNIT, 10);  // debounce 10 clocks
  // pcnt_filter_enable(PCNT_UNIT);

  // initialize PCNT count
  pcnt_counter_pause(PCNT_UNIT);
  pcnt_counter_clear(PCNT_UNIT);

  // enable PCNT overflow interrupt
  pcnt_event_enable(PCNT_UNIT, PCNT_EVT_H_LIM);
  // pcnt_event_enable(PCNT_UNIT, PCNT_EVT_L_LIM);
  pcnt_isr_register(onPcnt, NULL, ESP_INTR_FLAG_IRAM, &pcntisrHandle);
  pcnt_intr_enable(PCNT_UNIT);

  pcnt_counter_resume(PCNT_UNIT);  // start count
}

  // LEDC settings for gate signal
bool FreqCountESPgate::setupLedc(uint32_t freq, ledc_timer_bit_t resolution, uint32_t duty) {
  ledc_timer_config_t timer_conf = {
      .speed_mode       = PWM_MODE,
      .duty_resolution  = resolution,
      .timer_num        = LEDC_TIMER,
      .freq_hz          = freq, // temporal value
      .clk_cfg          = LEDC_USE_APB_CLK
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t ch_conf = {
      .gpio_num       = gate_pin,
      .speed_mode     = PWM_MODE,
      .channel        = LEDC_CHANNEL,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = LEDC_TIMER,
      .duty           = duty, // 1000ms or 100ms
      .hpoint         = 0
  };
  ledc_channel_config(&ch_conf);

  vTaskDelay(10); // to avoid Guru Meditation Error
  ledc_hal_context_t ledc_hal;
  ledc_hal_init(&ledc_hal, PWM_MODE);
  vTaskDelay(10); // to avoid Guru Meditation Error
  uint32_t div_q10_8 = 625 << 8; // 1 tic 1/128000 sec
  ledc_hal_set_clock_divider(&ledc_hal, LEDC_TIMER, div_q10_8);
  vTaskDelay(10); // to avoid Guru Meditation Error
  attachInterrupt(gate_pin, onLedc, FALLING);
  return true;
}

bool FreqCountESPgate::begin(uint16_t msec, uint8_t fpin, uint8_t gpin) {
  bool status;
  count_ovf = 0;
  fflag = false;
  first = true;
  gate_time = msec;
  gate_pin = gpin;
  setupPcnt(fpin, gpin);
  if (msec > 500) // 1sec
    status = setupLedc(1, LEDC_TIMER_17_BIT, 128000);  // 1Hz, 17bit, 1000ms
  else            // 0.1sec
    status = setupLedc(8, LEDC_TIMER_14_BIT, 12800);   // 8Hz, 14bit, 100ms
  if (!status) return false;
  const esp_timer_create_args_t timer_args = {
      .callback = &onDelay,
      .arg = NULL,
      .name = "delay_int"
  };
  esp_timer_create(&timer_args, &delay_int);
  return true;
}

uint32_t FreqCountESPgate::read() {
  uint32_t result;
  fflag = false;
  if (FreqCountESPgate::gate_time > 500)
    result = fcount;
  else
    result = fcount * 10;
  return result;
}

uint8_t FreqCountESPgate::available() {
  if (fflag) {
    if (first) {
      first = false;
      fflag = false;
      return 0;
    }
    return 1;
  } else {
    return 0;
  }
}

void FreqCountESPgate::end() {
  pcnt_counter_pause(PCNT_UNIT);
  pcnt_intr_disable(PCNT_UNIT);
  pcnt_isr_unregister(pcntisrHandle);
  detachInterrupt(gate_pin);
  // ledc_set_duty(PWM_MODE, LEDC_CHANNEL, 0);
  // ledc_update_duty(PWM_MODE, LEDC_CHANNEL);
  // ledc_timer_pause(PWM_MODE, LEDC_TIMER);
  esp_timer_stop(delay_int);
  esp_timer_delete(delay_int);
}

void FreqCountESPgate::pulse_test(uint8_t gpio_pin, uint32_t freq) {
  freq = constrain(freq, 1, 40000000);
  byte resolution = 0;
  for (long lfreq = 40000000; lfreq >= freq; ++resolution) {
    lfreq >>= 1;
  }
  resolution = constrain(resolution, 1, SOC_LEDC_TIMER_BIT_WIDTH);
  // Serial.print("resolution "); Serial.println(resolution);
  pinMode(gpio_pin, OUTPUT);

  // ledcAttachChannel(gpio_pin, 20000000, 2, PWM_CHANNEL);
  // ledcChangeFrequency(gpio_pin, freq, resolution);
  // ledcWrite(gpio_pin, 1 << (resolution - 1)); // duty 50%

  ledc_timer_config_t timer_conf = {
      .speed_mode       = PWM_MODE,
      .duty_resolution  = (ledc_timer_bit_t)resolution,
      .timer_num        = PWM_TIMER,
      .freq_hz          = freq, // temporal value
      .clk_cfg          = LEDC_USE_APB_CLK
  };
  ledc_timer_config(&timer_conf);
  vTaskDelay(10); // to avoid Guru Meditation Error

  ledc_channel_config_t ch_conf = {
      .gpio_num       = gpio_pin,
      .speed_mode     = PWM_MODE,
      .channel        = PWM_CHANNEL,
      .intr_type      = LEDC_INTR_DISABLE,
      .timer_sel      = PWM_TIMER,
      .duty           = 1 << (resolution - 1),
      .hpoint         = 0
  };
  ledc_channel_config(&ch_conf);
  vTaskDelay(10); // to avoid Guru Meditation Error

  // ledc_hal_context_t ledc_hal;
  // ledc_hal_init(&ledc_hal, PWM_MODE);
  // uint32_t div_q10_8 = (80000000LL << (8 - resolution)) / freq;
  // ledc_hal_set_clock_divider(&ledc_hal, PWM_TIMER, div_q10_8);
}

FreqCountESPgate FreqCount;
#endif
