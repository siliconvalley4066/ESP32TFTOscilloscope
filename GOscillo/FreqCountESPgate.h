#ifndef FreqCountESP_h
#define FreqCountESP_h

#include <Arduino.h>
#include "driver/pcnt.h"
#include "soc/pcnt_struct.h"
#include "esp_timer.h"

#define PULSE_INPUT_PIN 4
#define TIME_GATE_PIN 5
#define PCNT_UNIT PCNT_UNIT_0

void IRAM_ATTR onPcnt();
void IRAM_ATTR onLedc();
void IRAM_ATTR onTimer(void* arg);

class FreqCountESPgate {
private:
  void setupPcnt(uint8_t fpin, uint8_t gpin);
  bool setupLedc(uint32_t freq, uint8_t resolution, uint32_t duty);
  uint16_t gate_time;
  static uint8_t gate_pin;
  static bool first;
  pcnt_isr_handle_t pcntisrHandle;

public:
  static esp_timer_handle_t delay_int;
  static volatile uint32_t count_ovf;
  static volatile uint32_t fcount;
  static volatile bool fflag;
  static volatile uint32_t count_ovfx;
  static volatile uint16_t pulseCountx;
  bool begin(uint16_t msec, uint8_t fpin = PULSE_INPUT_PIN, uint8_t gpin = TIME_GATE_PIN);
  uint8_t available(void);
  uint32_t read(void);
  void end(void);
  void pulse_test(uint8_t gpio_pin, uint32_t freq);
};

extern FreqCountESPgate FreqCount;

#endif
