#ifndef FreqCountESP_h
#define FreqCountESP_h

#include <Arduino.h>
#include <driver/mcpwm_prelude.h>
#include <driver/pulse_cnt.h>
#include <driver/ledc.h>
#include <esp_timer.h>

#define PULSE_INPUT_PIN 4
#define TIME_GATE_PIN 5

bool IRAM_ATTR onPcntReach(pcnt_unit_handle_t unit,
                           const pcnt_watch_event_data_t *edata, void *user_ctx);
void IRAM_ATTR onMcPwm();
void IRAM_ATTR onDelay(void *arg);

class FreqCountESPgate {
private:
  void setupPcnt(uint8_t fpin, uint8_t gpin);
  void setupMCPWM(uint32_t freq, uint16_t period_ticks, uint32_t cmp_ticks);
  uint16_t gate_time;
  static uint8_t gate_pin;
  static pcnt_channel_handle_t pcnt_chan;
  static mcpwm_timer_handle_t timer_handle;
  static mcpwm_oper_handle_t oper_handle;
  static mcpwm_cmpr_handle_t cmp_handle;
  static mcpwm_gen_handle_t gen_handle;

public:
  static portMUX_TYPE mux;
  static pcnt_unit_handle_t pcnt_unit;
  static esp_timer_handle_t delay_int;
  static volatile uint32_t count_ovf;
  static volatile uint32_t fcount;
  static volatile bool fflag;
  bool begin(uint16_t msec, uint8_t fpin = PULSE_INPUT_PIN, uint8_t gpin = TIME_GATE_PIN);
  uint8_t available(void);
  uint32_t read(void);
  void end(void);
  void pulse_test(uint8_t gpio_pin, uint32_t freq);
};

extern FreqCountESPgate FreqCount;

#endif
