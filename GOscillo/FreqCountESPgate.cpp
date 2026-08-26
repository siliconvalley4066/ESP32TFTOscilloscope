#if !defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI) && !defined(ARDUINO_ESP32C3_DEV) && !defined(ARDUINO_WAVESHARE_ESP32_C3_ZERO)
/*
   ESP32 Frequency Counter Library Version 1.02
   The max frequency is 40MHz at 80MHz APB clock.
   Stable and accurate pulse counting by hardware gating.
   Copyright (c) 2026, Siliconvalley4066
   Licenced under the GNU GPL Version 3.0
*/
#include "FreqCountESPgate.h"

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
bool FreqCountESPgate::setupLedc(uint32_t freq, uint8_t resolution, uint32_t duty) {
  ledcSetClockSource((ledc_clk_cfg_t) LEDC_APB_CLK);
  if (!ledcAttachChannel(gate_pin, 8, 14, LEDC_CHANNEL_7))
    return false;
  ledcChangeFrequency(gate_pin, freq, resolution);
  vTaskDelay(2000);  // adhoc experiment reduce Guru Meditation Error since 3.3.4
  ledcWrite(gate_pin, duty);
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
    status = setupLedc(1, 17, 122880);  // 1Hz, 17bit, (1<<17)*15/16
  else            // 0.1sec
    status = setupLedc(8, 14, 12288);   // 8Hz, 14bit, (1<<14)*3/4
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
    result = (((long long)fcount << 17) * 1) / 122880;
  else
    result = (((long long)fcount << 14) * 8) / 12288;
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
  ledcDetach(gate_pin);
  esp_timer_stop(delay_int);
  esp_timer_delete(delay_int);
}

// double pulse_frq(void) {  // 4.768Hz <= pulse_frq <= 40MHz
//   return(80.0e6 / (1 << p_range) * count / 256.0);
// }

void FreqCountESPgate::pulse_test(uint8_t gpio_pin, uint32_t freq) {
  freq = constrain(freq, 1, 40000000);
  byte resolution = 0;
  for (long lfreq = 40000000; lfreq >= freq; ++resolution) {
    lfreq >>= 1;
  }
  resolution = constrain(resolution, 1, SOC_LEDC_TIMER_BIT_WIDTH);
  pinMode(gpio_pin, OUTPUT);
  ledcSetClockSource((ledc_clk_cfg_t) LEDC_APB_CLK);
  ledcAttach(gpio_pin, 20000000, 2);
  ledcChangeFrequency(gpio_pin, freq, resolution);
  ledcWrite(gpio_pin, 1 << (resolution - 1)); // duty 50%
}

FreqCountESPgate FreqCount;
#endif
