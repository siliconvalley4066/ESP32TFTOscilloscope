/*
 * DDS Sine Generator for ESP32
 * Modified by Siliconvalley4066 2022/4/22
 */
/*
 * DDS Sine Generator mit ATMEGS 168
 * Timer2 generates the  31250 KHz Clock Interrupt
 *
 * KHM 2009 /  Martin Nawrath
 * Kunsthochschule fuer Medien Koeln
 * Academy of Media Arts Cologne
 */

#include <driver/dac.h>
#include "soc/sens_reg.h"
#include "soc/rtc.h"

extern const unsigned char sine256[], saw256[], revsaw256[], triangle[], rect256[];
extern const unsigned char pulse20[], pulse10[], pulse05[], delta[], noise[];
extern const unsigned char gaussian_noise[], ecg[], sinc5[], sinc10[], sinc20[];
extern const unsigned char sine2harmonic[], sine3harmonic[], choppedsine[];
extern const unsigned char sinabs[], trapezoid[], step2[], step4[], chainsaw[];
unsigned char const *wp;
const unsigned char * wavetable[] PROGMEM = {sine256, saw256, revsaw256, triangle, rect256,
  pulse20, pulse10, pulse05, delta, noise, gaussian_noise, ecg, sinc5, sinc10, sinc20,
  sine2harmonic, sine3harmonic, choppedsine, sinabs, trapezoid, step2, step4, chainsaw};
const char Wavename[][5] PROGMEM = {"Sine", "Saw", "RSaw", "Tri", "Rect",
  "PL20", "PL10", "PL05", "Dlta", "Nois", "GNoi", "ECG", "Snc1", "Snc2", "Snc3",
  "Sin2", "Sin3", "CSin", "Sabs", "Trpz", "Stp2", "Stp4", "Csaw"};
const byte wave_num = (sizeof(wavetable) / sizeof(&sine256));
long ifreq = 12255; // frequency * 100 for 0.01Hz resolution
byte wave_id = 0;
#define DDSPin 25

// const double refclk=5000.0;  // 5kHz
const double refclk=5000.0;     // measured
// variables used inside interrupt service declared as voilatile
volatile byte icnt;              // var inside interrupt
volatile unsigned long phaccu;   // pahse accumulator
volatile unsigned long tword_m;  // dds tuning word m
volatile unsigned char wavebuf[256];
hw_timer_t * timer = NULL;

void dds_setup_init() {
  dac_output_enable(DAC_CHANNEL_1);
  if (dac_cw_mode)
    cw_dds_setup();
  else
    pwm_dds_setup();
}

void dds_setup() {
  if (dds_mode) return;
  dds_setup_init();
}

void pwm_dds_setup() {
  if (timer == NULL) {
    Setup_timer();
    tword_m=pow(2,32)*ifreq*0.01/refclk; // calulate DDS new tuning word
    wp = (unsigned char *) wavetable[wave_id];
    memcpy((void*)wavebuf, wp, 256);
  }
  timerAlarmEnable(timer);
}

void dds_close() {
  if (!dds_mode) return;
  if (dac_cw_mode) {
    dac_cw_generator_disable();
  } else {
    Close_timer();
  }
  dac_output_disable(DAC_CHANNEL_1);
}

void dds_set_freq() {
  double dfreq;
  dfreq = (double)ifreq*0.01;     // adjust output frequency
  tword_m=pow(2,32)*dfreq/refclk; // calulate DDS new tuning word
}

void rotate_wave(bool fwd) {
  if (fwd) {
    wave_id = (wave_id + 1) % wave_num;
  } else {
    if (wave_id > 0) --wave_id;
    else wave_id = wave_num - 1;
  }
  wp = (unsigned char *) wavetable[wave_id];
  memcpy((void*)wavebuf, wp, 256);
}

void set_wave(int id) {
  wave_id = id;
  wp = (unsigned char *) wavetable[wave_id];
  memcpy((void*)wavebuf, wp, 256);
}

//******************************************************************
// Timer Interrupt Service at 5 KHz = 200uSec
// this is the timebase REFCLOCK for the DDS generator
// FOUT = (M (REFCLK)) / (2 exp 32)
// runtime : ? microseconds ( inclusive push and pop)
void IRAM_ATTR onTimer() {
  phaccu=phaccu+tword_m; // soft DDS, phase accu with 32 bits
  icnt=phaccu >> 24;     // use upper 8 bits for phase accu as frequency information
                         // read value fron ROM sine table and send to PWM DAC
  dac_output_voltage(DAC_CHANNEL_1, wavebuf[icnt]);
//  dac_output_voltage(DAC_CHANNEL_1, wp[icnt]);
}

//******************************************************************
// timer setup
// 80000000/1000000*200 = 5.00 kHz clock
void Setup_timer() {
  timer = timerBegin(3, getApbFrequency()/1000000*200, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1, true);
  timerAlarmEnable(timer);
}

void Close_timer() {
  timerAlarmDisable(timer);
//  timerEnd(timer);
//  timer = NULL;
}

void update_ifrq(long diff) {
  long newFreq;
  int fast;
  if (diff != 0) {
    if (abs(diff) > 3) {
      fast = ifreq / 40;
    } else if (abs(diff) > 2) {
      fast = ifreq / 300;
    } else if (abs(diff) > 1) {
      fast = 25;
    } else {
      fast = 1;
    }
    if (fast < 1) fast = 1;
    newFreq = ifreq + fast * diff;
  } else {
    newFreq = ifreq;
  }
  if (dac_cw_mode && newFreq < 1) {             // switch to PWM mode
    dac_cw_generator_disable();
    dac_cw_mode = false;
    newFreq = 12969;  // 129.69Hz
    pwm_dds_setup();
  } else if (!dac_cw_mode && newFreq > 51880) { // switch to CW mode
    Close_timer();
    dac_cw_mode = true;
    newFreq = 4;      // 518.81Hz
    cw_dds_setup();
  }
  newFreq = constrain(newFreq, 1, 99999);
  if (newFreq != ifreq) {
    ifreq = newFreq;
    if (dac_cw_mode)
      SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, ifreq, SENS_SW_FSTEP_S);
    else
      dds_set_freq();
  }
}

float set_freq(float dfreq) {
  long newfreq = 100.0 * dfreq;
  if (dac_cw_mode && newfreq < 12970) {         // switch to PWM mode
    dac_cw_generator_disable();
    dac_cw_mode = false;
    ifreq = round(newfreq);
    pwm_dds_setup();
  } else if (!dac_cw_mode && newfreq > 51879) { // switch to CW mode
    Close_timer();
    dac_cw_mode = true;
    ifreq = dfreq * 65536.0 / RTC_FAST_CLK_FREQ_APPROX;
    cw_dds_setup();
  }
  if (dac_cw_mode) {
    ifreq = round(dfreq * 65536.0 / RTC_FAST_CLK_FREQ_APPROX);
    ifreq = constrain(ifreq, 1, 99999);
    SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, ifreq, SENS_SW_FSTEP_S);
    dfreq = RTC_FAST_CLK_FREQ_APPROX * (float) ifreq / 65536.0;
  } else {
    ifreq = constrain(newfreq, 1, 99999);
    dds_set_freq();
    dfreq = 0.01 * ifreq;
  }
  return dfreq;
}

float dds_freq(void) {
  float frequency;
  if (dac_cw_mode) {
    frequency = RTC_FAST_CLK_FREQ_APPROX * (float) ifreq / 65536.0;
  } else {
    frequency = (float)ifreq * 0.01;
  }
  return frequency;
}

#ifndef NOLCD
void disp_dds_freq(void) {
  if (dac_cw_mode) {
    float frequency = RTC_FAST_CLK_FREQ_APPROX * (float) ifreq / 65536.0;
    if (frequency < 100000.0)
      display.print(frequency, 2);
    else
      display.print(frequency, 0);
  } else {
    display.print((float)ifreq * 0.01, 2);
  }
  display.print("Hz");
}

void disp_dds_wave(void) {
  display.print(Wavename[wave_id]); 
}
#endif

void cw_dds_setup() {
  dac_cw_generator_enable();
  dac_cw_config_t cw = {
    .en_ch = DAC_CHANNEL_1,
    .scale = DAC_CW_SCALE_1,  // DAC_CW_SCALE_2:1/2 DAC_CW_SCALE_4:1/4 DAC_CW_SCALE_8:1/8
    .phase = DAC_CW_PHASE_0,  // DAC_CW_PHASE_0:0degree DAC_CW_PHASE_180:+180degree
    .freq = (uint32_t) 130,   // 130(130Hz) ~ 65537(65.537kHz why uint32?)
    .offset = (int8_t) 0      // 0 yields Bug
  };
  dac_cw_generator_config(&cw);
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, 0, SENS_DAC_DC1_S);  // fix offset bug
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, ifreq, SENS_SW_FSTEP_S);
}
