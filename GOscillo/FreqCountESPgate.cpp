#if !defined(ARDUINO_NOLOGO_ESP32C3_SUPER_MINI) && !defined(ARDUINO_ESP32C3_DEV) && !defined(ARDUINO_WAVESHARE_ESP32_C3_ZERO)
/*
   ESP32 Frequency Counter Library Version 1.06
   The max frequency is 40MHz at 80MHz APB clock.
   Stable and accurate pulse counting by hardware gating.
   Copyright (c) 2026, Siliconvalley4066
   Licenced under the GNU GPL Version 3.0
*/
#include "FreqCountESPgate.h"

#define PWM_CHANNEL LEDC_CHANNEL_7
#define PWM_TIMER LEDC_TIMER_3
#define PWM_MODE LEDC_HIGH_SPEED_MODE

volatile uint32_t FreqCountESPgate::count_ovf;
volatile uint32_t FreqCountESPgate::fcount;
volatile bool FreqCountESPgate::fflag;
uint8_t FreqCountESPgate::gate_pin = TIME_GATE_PIN;
esp_timer_handle_t FreqCountESPgate::delay_int = NULL;
pcnt_unit_handle_t FreqCountESPgate::pcnt_unit = NULL;
pcnt_channel_handle_t FreqCountESPgate::pcnt_chan = NULL;
mcpwm_timer_handle_t FreqCountESPgate::timer_handle = NULL;
mcpwm_oper_handle_t FreqCountESPgate::oper_handle = NULL;
mcpwm_cmpr_handle_t FreqCountESPgate::cmp_handle = NULL;
mcpwm_gen_handle_t FreqCountESPgate::gen_handle = NULL;

portMUX_TYPE FreqCountESPgate::mux = portMUX_INITIALIZER_UNLOCKED;

bool IRAM_ATTR onPcntReach(
  pcnt_unit_handle_t unit,
  const pcnt_watch_event_data_t *edata,
  void *user_ctx) {
  portENTER_CRITICAL_ISR(&FreqCountESPgate::mux);
  FreqCountESPgate::count_ovf++;  // high limit overflow
  portEXIT_CRITICAL_ISR(&FreqCountESPgate::mux);
  return false;
}

void IRAM_ATTR onMcPwm() {
  // Start a one-shot timer to run after 100 us
  esp_timer_start_once(FreqCountESPgate::delay_int, 100);  // 100us
}

void IRAM_ATTR onDelay(void *arg) {
  // This is called after 100 us onMcPwm()
  int pulseCount;
  uint32_t ovf;
  pcnt_unit_get_count(FreqCountESPgate::pcnt_unit, &pulseCount);

  portENTER_CRITICAL(&FreqCountESPgate::mux);
  ovf = FreqCountESPgate::count_ovf;
  portEXIT_CRITICAL(&FreqCountESPgate::mux);

  FreqCountESPgate::fcount = (uint32_t)pulseCount + ovf * 32767;
  pcnt_unit_clear_count(FreqCountESPgate::pcnt_unit);

  portENTER_CRITICAL(&FreqCountESPgate::mux);
  FreqCountESPgate::count_ovf = 0;
  portEXIT_CRITICAL(&FreqCountESPgate::mux);

  FreqCountESPgate::fflag = true;
}

void FreqCountESPgate::setupPcnt(uint8_t fpin, uint8_t gpin) {
  pcnt_unit_config_t uc = {};

  uc.low_limit = -32768;
  uc.high_limit = 32767;
  pcnt_new_unit(&uc, &FreqCountESPgate::pcnt_unit);

  pcnt_chan_config_t cc = {};
  cc.edge_gpio_num = fpin;
  cc.level_gpio_num = gpin;
  pcnt_new_channel(FreqCountESPgate::pcnt_unit, &cc, &pcnt_chan);
  pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                               PCNT_CHANNEL_EDGE_ACTION_HOLD);
  // Gate=0 → HOLD
  // Gate=1 → COUNT
  pcnt_channel_set_level_action(pcnt_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                PCNT_CHANNEL_LEVEL_ACTION_HOLD);

  pcnt_event_callbacks_t cbs = {};
  cbs.on_reach = onPcntReach;
  pcnt_unit_register_event_callbacks(FreqCountESPgate::pcnt_unit, &cbs, NULL);
  pcnt_unit_add_watch_point(FreqCountESPgate::pcnt_unit, 32767);
  pcnt_unit_enable(FreqCountESPgate::pcnt_unit);
  pcnt_unit_start(FreqCountESPgate::pcnt_unit);
}

// MCPWM settings for gate signal
void FreqCountESPgate::setupMCPWM(uint32_t freq, uint16_t period_ticks, uint32_t cmp_ticks) {
  mcpwm_timer_config_t tc = {};
  tc.group_id = 0;
  tc.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
  tc.resolution_hz = freq;  // 50kHz timebase 1tick = 20us
  tc.count_mode = MCPWM_TIMER_COUNT_MODE_UP;
  tc.period_ticks = period_ticks;  // 50500tick = 1.01000s : 1.00000s High 10ms Low
  mcpwm_new_timer(&tc, &timer_handle);
  mcpwm_operator_config_t oc = {};
  oc.group_id = 0;
  mcpwm_new_operator(&oc, &oper_handle);
  mcpwm_operator_connect_timer(oper_handle, timer_handle);
  mcpwm_comparator_config_t cc = {};
  mcpwm_new_comparator(oper_handle, &cc, &cmp_handle);
  mcpwm_comparator_set_compare_value(cmp_handle, cmp_ticks);
  mcpwm_generator_config_t gc = {};
  gc.gen_gpio_num = gate_pin;
  mcpwm_new_generator(oper_handle, &gc, &gen_handle);
  mcpwm_generator_set_action_on_timer_event(
    gen_handle,
    MCPWM_GEN_TIMER_EVENT_ACTION(
      MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
  mcpwm_generator_set_action_on_compare_event(
    gen_handle,
    MCPWM_GEN_COMPARE_EVENT_ACTION(
      MCPWM_TIMER_DIRECTION_UP, cmp_handle, MCPWM_GEN_ACTION_LOW));
  mcpwm_timer_enable(timer_handle);
  // mcpwm_timer_start_stop(timer_handle, MCPWM_TIMER_START_NO_STOP);
}

bool FreqCountESPgate::begin(uint16_t msec, uint8_t fpin, uint8_t gpin) {
  count_ovf = 0;
  fflag = false;
  msec = constrain(msec, 1, 16373);  // max (65535 - 40) / 4
  gate_time = msec;
  gate_pin = gpin;
  setupMCPWM(4000, 4 * msec + 40, 4 * msec);  // tick=0.25ms : 4*msec High 10ms Low
  attachInterrupt(gate_pin, onMcPwm, FALLING);
  setupPcnt(fpin, gpin);
  mcpwm_timer_start_stop(timer_handle, MCPWM_TIMER_START_NO_STOP);
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
  return fcount;
}

uint8_t FreqCountESPgate::available() {
  if (fflag) {
    return 1;
  } else {
    return 0;
  }
}

void FreqCountESPgate::end() {
  detachInterrupt(gate_pin);
  esp_timer_stop(delay_int);
  esp_timer_delete(delay_int);
  pcnt_unit_stop(pcnt_unit);
  pcnt_unit_disable(pcnt_unit);
  pcnt_del_channel(pcnt_chan);
  pcnt_del_unit(pcnt_unit);
  mcpwm_timer_start_stop(timer_handle, MCPWM_TIMER_STOP_FULL);
  mcpwm_timer_disable(timer_handle);
  mcpwm_del_generator(gen_handle);
  mcpwm_del_comparator(cmp_handle);
  mcpwm_del_operator(oper_handle);
  mcpwm_del_timer(timer_handle);
  delay_int = NULL;
  pcnt_chan = NULL;
  pcnt_unit = NULL;
  gen_handle = NULL;
  cmp_handle = NULL;
  oper_handle = NULL;
  timer_handle = NULL;
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
    .speed_mode = PWM_MODE,
    .duty_resolution = (ledc_timer_bit_t)resolution,
    .timer_num = PWM_TIMER,
    .freq_hz = freq,  // temporal value
    .clk_cfg = LEDC_USE_APB_CLK
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t ch_conf = {
    .gpio_num = gpio_pin,
    .speed_mode = PWM_MODE,
    .channel = PWM_CHANNEL,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = PWM_TIMER,
    .duty = 1 << (resolution - 1),
    .hpoint = 0
  };
  ledc_channel_config(&ch_conf);

  // ledc_hal_context_t ledc_hal;
  // ledc_hal_init(&ledc_hal, PWM_MODE);
  // uint32_t div_q10_8 = (80000000LL << (8 - resolution)) / freq;
  // ledc_hal_set_clock_divider(&ledc_hal, PWM_TIMER, div_q10_8);
}

FreqCountESPgate FreqCount;
#endif
