/*
 * ESP32 Oscilloscope using a 320x240 TFT Version 1.01
 * The max realtime sampling rates are 10ksps with 2 channels and 20ksps with a channel.
 * In the I2S DMA mode, it can be set up to 500ksps, however effective samplig rate is 200ksps.
 * + Pulse Generator
 * + PWM DDS Function Generator (23 waveforms)
 * Copyright (c) 2022, Siliconvalley4066
 */
/*
 * Arduino Oscilloscope using a graphic LCD
 * The max sampling rates are 4.3ksps with 2 channels and 8.6ksps with a channel.
 * Copyright (c) 2009, Noriaki Mitsunaga
 */

#include <SPI.h>
#include "TFT_eSPI.h"
#include "driver/adc.h"
//#include "esp_task_wdt.h"

TFT_eSPI display = TFT_eSPI();

//#define BUTTON5DIR
#include <EEPROM.h>
#include "arduinoFFT.h"
#define FFT_N 256
arduinoFFT FFT = arduinoFFT();  // Create FFT object

#define txtLINE0   0
#define txtLINE1   8
#define txtLINE2   16
#define txtLINE3   24
#define txtLINE4   32
#define txtLINE5   40
#define txtLINE6   48
#define txtLINE7   56
#define DISPTXT 296

float waveFreq;                // frequency (Hz)
float waveDuty;                // duty ratio (%)
int dataMin;                   // buffer minimum value (smallest=0)
int dataMax;                   //        maximum value (largest=1023)
int dataAve;                   // 10 x average value (use 10x value to keep accuracy. so, max=10230)
int saveTimer;                 // remaining time for saving EEPROM
int timeExec;                  // approx. execution time of current range setting (ms)
extern byte duty;
extern byte p_range;
extern unsigned short count;
extern long ifreq;
extern byte wave_id;

const int LCD_WIDTH = 320;
const int LCD_HEIGHT = 240;
const int LCD_YMAX = 200;
const int SAMPLES = 300;
const int NSAMP = 1024;
const int DISPLNG = 300;
const int DOTS_DIV = 25;
const int XOFF = 10;
const int YOFF = 20;
const int ad_ch0 = ADC1_CHANNEL_6;  // Analog 34 pin for channel 0 ADC1_CHANNEL_6
const int ad_ch1 = ADC1_CHANNEL_7;  // Analog 35 pin for channel 1 ADC1_CHANNEL_7
const long VREF[] = {83, 165, 413, 825, 1650}; // reference voltage 3.3V ->  82.5 :   1V/div range (40mV/dot)
                                        //                        -> 165 : 0.5V/div
                                        //                        -> 413 : 0.2V/div
                                        //                        -> 825 : 100mV/div
                                        //                        -> 1650 : 50mV/div
//const int MILLIVOL_per_dot[] = {100, 50, 20, 10, 5}; // mV/dot
//const int ac_offset[] PROGMEM = {1792, -128, -1297, -1679, -1860}; // for OLED
const int ac_offset[] PROGMEM = {3072, 512, -1043, -1552, -1804}; // for Web
const int MODE_ON = 0;
const int MODE_INV = 1;
const int MODE_OFF = 2;
const char Modes[3][4] PROGMEM = {"ON", "INV", "OFF"};
const int TRIG_AUTO = 0;
const int TRIG_NORM = 1;
const int TRIG_SCAN = 2;
const int TRIG_ONE  = 3;
const char TRIG_Modes[4][5] PROGMEM = {"Auto", "Norm", "Scan", "One "};
const int TRIG_E_UP = 0;
const int TRIG_E_DN = 1;
#define RATE_MIN 0
#define RATE_MAX 18
#define RATE_NUM 19
#define RATE_DMA 5
#define RATE_DUAL 7
#define RATE_ROLL 15
#define ITEM_MAX 28
const char Rates[RATE_NUM][5] PROGMEM = {"50us", "100u", "250u", "333u", "500u", " 1ms", "1.3m", " 2ms", " 5ms", "10ms", "20ms", "50ms", "100m", "200m", "0.5s", " 1s ", " 2s ", " 5s ", " 10s"};
const unsigned long HREF[] PROGMEM = {20, 40, 100, 133, 200, 400, 500, 800, 2000, 4000, 8000, 20000, 40000, 80000, 200000, 400000, 800000, 2000000, 4000000};
#define RANGE_MIN 0
#define RANGE_MAX 4
#define VRF 3.3
const char Ranges[5][5] PROGMEM = {" 1V ", "0.5V", "0.2V", "0.1V", "50mV"};
byte range0 = RANGE_MIN;
byte range1 = RANGE_MIN;
byte ch0_mode = MODE_ON, ch1_mode = MODE_ON, rate = 0, orate;
byte trig_mode = TRIG_AUTO, trig_lv = 10, trig_edge = TRIG_E_UP, trig_ch = ad_ch0;
bool Start = true;  // Start sampling
byte item = 0;      // Default item
byte menu = 0;      // Default menu
short ch0_off = 0, ch1_off = 400;
byte data[4][SAMPLES];                  // keep twice of the number of channels to make it a double buffer
uint16_t cap_buf[NSAMP], cap_buf1[SAMPLES];
uint16_t payload[SAMPLES*2];
byte odat00, odat01, odat10, odat11;    // old data buffer for erase
byte sample=0;                          // index for double buffer
bool fft_mode = false, pulse_mode = false, dds_mode = false, fcount_mode = false;
bool full_screen = false;
byte info_mode = 3; // Text information display mode
bool dac_cw_mode = false;
int trigger_ad;

//#define LED_BUILTIN 2
#define LEFTPIN   12  // LEFT
#define RIGHTPIN  13  // RIGHT
#define UPPIN     14  // UP
#define DOWNPIN   27  // DOWN
#define CH0DCSW   33  // DC/AC switch ch0
#define CH1DCSW   32  // DC/AC switch ch1
//#define I2CSDA    21  // I2C SDA
//#define I2CSCL    22  // I2C SCL
// DAC_CHANNEL_1  is GPIO25
// DAC_CHANNEL_2  is GPIO26
// ADC1_CHANNEL_0 is GPIO36
// ADC1_CHANNEL_1 is GPIO37
// ADC1_CHANNEL_2 is GPIO38
// ADC1_CHANNEL_3 is GPIO39
// ADC1_CHANNEL_4 is GPIO32
// ADC1_CHANNEL_5 is GPIO33
// ADC1_CHANNEL_6 is GPIO34
// ADC1_CHANNEL_7 is GPIO35
#define BGCOLOR   TFT_BLACK
#define GRIDCOLOR TFT_DARKGREY
#define CH1COLOR  TFT_GREEN
#define CH2COLOR  TFT_YELLOW
#define FRMCOLOR  TFT_LIGHTGREY
#define TXTCOLOR  TFT_WHITE

TaskHandle_t taskHandle;

void setup(){
  xTaskCreatePinnedToCore(setup1, "WebProcess", 4096, NULL, 1, &taskHandle, PRO_CPU_NUM); //Core 0でタスク開始
  pinMode(CH0DCSW, INPUT_PULLUP);   // CH1 DC/AC
  pinMode(CH1DCSW, INPUT_PULLUP);   // CH2 DC/AC
  pinMode(UPPIN, INPUT_PULLUP);     // up
  pinMode(DOWNPIN, INPUT_PULLUP);   // down
  pinMode(RIGHTPIN, INPUT_PULLUP);  // right
  pinMode(LEFTPIN, INPUT_PULLUP);   // left
  pinMode(34, ANALOG);              // Analog 34 pin for channel 0 ADC1_CHANNEL_6
  pinMode(35, ANALOG);              // Analog 35 pin for channel 1 ADC1_CHANNEL_7
//  pinMode(LED_BUILTIN, OUTPUT);     // sets the digital pin as output
  display.init();                    // initialise the library
  display.setRotation(1);
  uint16_t calData[5] = { 368, 3538, 256, 3459, 7 };
  display.setTouch(calData);

//  Serial.begin(115200);
//  Serial.printf("CORE1 = %d\n", xPortGetCoreID());
  EEPROM.begin(32);                     // set EEPROM size. Necessary for ESP32
  loadEEPROM();                         // read last settings from EEPROM
//  set_default();
  menu = item >> 3;
  display.fillScreen(BGCOLOR);
//  DrawGrid();
//  DrawText();
//  display.display();
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);
  adc1_config_width(ADC_WIDTH_BIT_12);
  if (pulse_mode)
    pulse_init();                       // calibration pulse output
  if (dds_mode)
    dds_setup_init();
  orate = RATE_DMA + 1;                 // old rate befor change
  rate_i2s_mode_config();
}

byte lastsw = 255;
unsigned long vtime;

void CheckSW() {
  static unsigned long Millis = 0;
  unsigned long ms;
  byte sw;

  ms = millis();
  if ((ms - Millis)<200)
    return;
  Millis = ms;

  CheckTouch();

/* SW10 Menu
 * SW9  CH1 range down
 * SW8  CH2 range down
 * SW7  TIME/DIV slow
 * SW6  TRIG_MODE down
 * SW5  Send
 * SW4  TRIG_MODE up
 * SW3  TIME/DIV fast
 * SW2  CH2 range up
 * SW1  CH1 range up
 * SW0  Start/Hold
 */
#ifdef BUTTON5DIR
  if (digitalRead(DOWNPIN) == LOW && digitalRead(LEFTPIN) == LOW) {
    sw = 11;    // both button press
  } else if (digitalRead(UPPIN) == LOW && digitalRead(RIGHTPIN) == LOW) {
    sw = 12;    // both button press
#else
  if (digitalRead(RIGHTPIN) == LOW && digitalRead(LEFTPIN) == LOW) {
    sw = 11;    // both button press
  } else if (digitalRead(UPPIN) == LOW && digitalRead(DOWNPIN) == LOW) {
    sw = 12;    // both button press
#endif
  } else if (digitalRead(DOWNPIN) == LOW) {
    sw = 10;    // down
  } else if (digitalRead(RIGHTPIN) == LOW) {
    sw = 3;     // right
  } else if (digitalRead(LEFTPIN) == LOW) {
    sw = 7;     // left
  } else if (digitalRead(UPPIN) == LOW) {
    sw = 0;     // up
  } else {
    lastsw = 255;
    return;
  }
  if (sw != lastsw)
    vtime = ms;
  saveTimer = 5000;     // set EEPROM save timer to 5 secnd
  if (sw == 12) {
    full_screen = !full_screen;
    display.fillRect(DISPTXT,0,25,64, BGCOLOR);  // clear text area that will be drawn below 
  } else {
    switch (menu) {
    case 0:
      menu0_sw(sw); 
      break;
    case 1:
      menu1_sw(sw); 
      break;
    case 2:
      menu2_sw(sw); 
      break;
    case 3:
      menu3_sw(sw); 
      break;
    default:
      break;
    }
    DrawText();
//    display.display();
  }
  lastsw = sw;
}

void updown_ch0range(byte sw) {
  if (sw == 3) {        // CH0 RANGE +
    if (range0 > 0)
      range0 --;
  } else if (sw == 7) { // CH0 RANGE -
    if (range0 < RANGE_MAX)
      range0 ++;
  }
}

void updown_ch1range(byte sw) {
  if (sw == 3) {        // CH1 RANGE +
    if (range1 > 0)
      range1 --;
  } else if (sw == 7) { // CH1 RANGE -
    if (range1 < RANGE_MAX)
      range1 ++;
  }
}

void updown_rate(byte sw) {
  if (sw == 3) {        // RATE FAST
    orate = rate;
    if (rate > 0) {
      rate --;
      rate_i2s_mode_config();
    }
    display.fillScreen(BGCOLOR);
  } else if (sw == 7) { // RATE SLOW
    orate = rate;
    if (rate < RATE_MAX) {
      rate ++;
      rate_i2s_mode_config();
    } else {
      rate = RATE_MAX;
    }
  }
}

void menu0_sw(byte sw) {  
  switch (item) {
  case 0: // CH0 voltage range
    updown_ch0range(sw);
    break;
  case 1: // CH1 voltage range
    updown_ch1range(sw);
    break;
  case 2: // rate
    updown_rate(sw);
    break;
  case 3: // sampling mode
    break;
  case 4: // trigger mode
    if (sw == 3) {        // TRIG MODE +
      if (trig_mode < TRIG_ONE)
        trig_mode ++;
      else
        trig_mode = 0;
    } else if (sw == 7) { // TRIG MODE -
      if (trig_mode > 0)
        trig_mode --;
      else
        trig_mode = TRIG_ONE;
    }
    if (trig_mode != TRIG_ONE)
        Start = true;
    break;
  case 5: // trigger source and polarity
    if (sw == 3) {        // trigger + edge
      if (trig_edge == TRIG_E_UP)
        trig_edge = TRIG_E_DN;
      else
        trig_edge = TRIG_E_UP;
    } else if (sw == 7) { // trigger - channel
      if (trig_ch == ad_ch0)
        trig_ch = ad_ch1;
      else
        trig_ch = ad_ch0;
      set_trigger_ad();
    }
    break;
  case 6: // trigger level
    if (sw == 3) {        // trigger level +
      draw_trig_level(BGCOLOR); // erase old trig_lv mark
      if (trig_lv < LCD_YMAX) {
        trig_lv ++;
        set_trigger_ad();
      }
    } else if (sw == 7) { // trigger level -
      draw_trig_level(BGCOLOR); // erase old trig_lv mark
      if (trig_lv > 0) {
        trig_lv --;
        set_trigger_ad();
      }
    }
    break;
  case 7: // run / hold
    if (sw == 3 || sw == 7) {
      Start = !Start;
    }
    break;
  }
  menu_updown(sw);
}

void menu1_sw(byte sw) {  
  switch (item - 8) {
  case 1: // CH0 mode
    if (sw == 3) {        // CH0 + ON/INV
      if (ch0_mode == MODE_ON)
        ch0_mode = MODE_INV;
      else
        ch0_mode = MODE_ON;
    } else if (sw == 7) { // CH0 - ON/OFF
      if (ch0_mode == MODE_OFF)
        ch0_mode = MODE_ON;
      else {
        ch0_mode = MODE_OFF;
        display.fillScreen(BGCOLOR);
      }
    }
    break;
  case 2: // CH0 voltage range
    updown_ch0range(sw);
    break;
  case 3: // CH0 offset
    if (sw == 3) {        // offset +
      if (ch0_off < 8191)
        ch0_off += 4096/VREF[range0];
    } else if (sw == 7) { // offset -
      if (ch0_off > -8191)
        ch0_off -= 4096/VREF[range0];
    } else if (sw == 11) { // offset reset
      if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
        ch0_off = ac_offset[range0];
      else
        ch0_off = 0;
    }
    break;
  case 5: // CH1 mode
    if (sw == 3) {        // CH1 + ON/INV
      if (ch1_mode == MODE_ON)
        ch1_mode = MODE_INV;
      else
        ch1_mode = MODE_ON;
    } else if (sw == 7) { // CH1 - ON/OFF
      if (ch1_mode == MODE_OFF)
        ch1_mode = MODE_ON;
      else {
        ch1_mode = MODE_OFF;
        display.fillScreen(BGCOLOR);
      }
    }
    break;
  case 6: // CH1 voltage range
    updown_ch1range(sw);
    break;
  case 7: // CH1 offset
    if (sw == 3) {        // offset +
      if (ch1_off < 8191)
        ch1_off += 4096/VREF[range1];
    } else if (sw == 7) { // offset -
      if (ch1_off > -8191)
        ch1_off -= 4096/VREF[range1];
    } else if (sw == 11) { // offset reset
      if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
        ch1_off = ac_offset[range1];
      else
        ch1_off = 0;
    }
    break;
  }
  menu_updown(sw);
}

void menu2_sw(byte sw) {
  int diff;
  switch (item - 16) {
  case 0: // CH0 voltage range
    updown_ch0range(sw);
    break;
  case 1: // rate
    updown_rate(sw);
    break;
  case 2: // FFT mode
    if (sw == 3) {        // ON
      fft_mode = true;
      display.fillScreen(BGCOLOR);
    } else if (sw == 7) { // OFF
      fft_mode = false;
      display.fillScreen(BGCOLOR);
    }
    break;
  case 3: // Frequency and Duty display
    if (sw == 3) {        // ON
      info_mode |= 1;
    } else if (sw == 7) { // OFF
      info_mode &= ~1;
    }
    break;
  case 4: // Voltage display
    if (sw == 3) {        // ON
      info_mode |= 2;
    } else if (sw == 7) { // OFF
      info_mode &= ~2;
    }
    break;
  case 5: // PWM
    if (sw == 3) {        // +
      update_frq(0);
      pulse_start();
      pulse_mode = true;
    } else if (sw == 7) { // -
      pulse_close();
      pulse_mode = false;
    }
    break;
  case 6: // PWM Duty ratio
    diff = 1;
    if (sw == lastsw) {
      if (millis() - vtime > 5000) diff = 8;
    }
    if (sw == 3) {        // +
      if (pulse_mode) {
        if ((256 - duty) > diff) duty += diff;
      } else {
        pulse_start();
      }
      update_frq(0);
      pulse_mode = true;
    } else if (sw == 7) { // -
      if (pulse_mode) {
        if (duty > diff) duty -= diff;
      } else {
        pulse_start();
      }
      update_frq(0);
      pulse_mode = true;
    }
    break;
  case 7: // PWM Frequency
    diff = sw_accel(sw);
    if (sw == 3) {        // +
      if (pulse_mode)
        update_frq(-diff);
      else {
        update_frq(0);
        pulse_start();
      }
      pulse_mode = true;
    } else if (sw == 7) { // -
      if (pulse_mode)
        update_frq(diff);
      else {
        update_frq(0);
        pulse_start();
      }
      pulse_mode = true;
    }
    break;
  }
  menu_updown(sw);
}

void menu3_sw(byte sw) {
  char diff;
  switch (item - 24) {
  case 0: // CH0 voltage range
    updown_ch0range(sw);
    break;
  case 1: // rate
    updown_rate(sw);
    break;
  case 2: // DDS
    if (sw == 3) {        // +
      dds_setup();
      dds_mode = true;
    } else if (sw == 7) { // -
      dds_close();
      dds_mode = false;
      display.fillRect(DISPTXT-7*6,56,66,8, BGCOLOR); // clear text area of the DDS frequency
    }
    break;
  case 3: // WAVE
    if (sw == 3) {        // +
      rotate_wave(true);
    } else if (sw == 7) { // -
      rotate_wave(false);
    }
    break;
  case 4: // FREQ
    diff = sw_accel(sw);
    if (sw == 3) {        // +
      update_ifrq(diff);
    } else if (sw == 7) { // -
      update_ifrq(-diff);
    }
    break;
  case 5: // Frequency Counter
    if (sw == 3 && rate <= RATE_MAX) {  // on
//      dds_close(); dds_mode = false;
//      pulse_close();
      pulse_mode = false; fcount_mode = true;
//      FreqCount.begin(1000);
    } else if (sw == 7) {               // off
      fcount_close();
    }
    break;
  }
  menu_updown(sw);
}

void menu_updown(byte sw) {
  if (sw == 10) {       // MENU down SW
    increment_item();
  } else if (sw == 0) { // Menu up SW
    decrement_item();
  }
}

void increment_item() {
  ++item;
  if (item > ITEM_MAX) item = 0;
  if (item == 3) item = 4;      // skip real/DMA
  if (item < 16 || item > 18) fft_mode = false; // exit FFT mode
  menu = item >> 3;
}

void decrement_item() {
  if (item > 0) --item;
  else item = ITEM_MAX;
  if (item == 3) item = 2;      // skip real/DMA
  if (item < 16 || item > 18) fft_mode = false; // exit FFT mode
  menu = item >> 3;
}

byte sw_accel(byte sw) {
  char diff = 1;
  if (sw == lastsw) {
    unsigned long curtime = millis();
    if (curtime - vtime > 6000) diff = 4;
    else if (curtime - vtime > 4000) diff = 3;
    else if (curtime - vtime > 2000) diff = 2;
  }
  return (diff);
}

#ifdef DOT_GRID
void DrawGrid() {
  int disp_leng;
  if (full_screen) disp_leng = SAMPLES;
  else disp_leng = DISPLNG;
  for (int x=0; x<=disp_leng; x += 5) { // Horizontal Line
    for (int y=0; y<=LCD_YMAX; y += DOTS_DIV) {
      display.drawPixel(XOFF+x, YOFF+y, GRIDCOLOR);
//      CheckSW();
    }
  }
  for (int x=0; x<=disp_leng; x += DOTS_DIV ) { // Vertical Line
    for (int y=0; y<=LCD_YMAX; y += 5) {
      display.drawPixel(XOFF+x, YOFF+y, GRIDCOLOR);
//      CheckSW();
    }
  }
}
#else
void DrawGrid() {
  display.drawFastVLine(XOFF, YOFF, LCD_YMAX, FRMCOLOR);          // left vertical line
  display.drawFastVLine(XOFF+SAMPLES, YOFF, LCD_YMAX, FRMCOLOR);  // right vertical line
  display.drawFastHLine(XOFF, YOFF, SAMPLES, FRMCOLOR);           // top horizontal line
  display.drawFastHLine(XOFF, YOFF+LCD_YMAX, SAMPLES, FRMCOLOR);  // bottom horizontal line

  for (int y = 0; y < LCD_YMAX; y += DOTS_DIV) {
    if (y > 0){
      display.drawFastHLine(XOFF+1, YOFF+y, SAMPLES - 1, GRIDCOLOR);  // Draw 9 horizontal lines
    }
    for (int i = 5; i < DOTS_DIV; i += 5) {
      display.drawFastHLine(XOFF+SAMPLES / 2 - 3, YOFF+y + i, 7, GRIDCOLOR);  // Draw the vertical center line ticks
    }
  }
  for (int x = 0; x < SAMPLES; x += DOTS_DIV) {
    if (x > 0){
      display.drawFastVLine(XOFF+x, YOFF+1, LCD_YMAX - 1, GRIDCOLOR); // Draw 11 vertical lines
    }
    for (int i = 5; i < DOTS_DIV; i += 5) {
      display.drawFastVLine(XOFF+x + i, YOFF+LCD_YMAX / 2 -3, 7, GRIDCOLOR);  // Draw the horizontal center line ticks
    }
  }
}
#endif

void DrawText() {
  display.fillRect(DISPTXT,0,25,64, BGCOLOR); // clear text area that will be drawn below 
  display.setTextSize(1);

  switch (menu) {
  case 0:
    set_line_color(0);
    if (ch0_mode != MODE_OFF) {
      display_range(range0);
    } else {
      display.print(F("CH2")); display_ac(CH1DCSW);
    }
    set_line_color(1);
    if (ch1_mode != MODE_OFF && rate >= RATE_DUAL) {
      display_range(range1);
    } else {
      display.print(F("CH1")); display_ac(CH0DCSW);
    }
    set_line_color(2);
    display_rate();
    set_line_color(3);
    if (rate > RATE_DMA) display.print(F("real"));
    else display.print(F("DMA"));
    set_line_color(4);
    display_trig_mode();
    set_line_color(5);
    display.print(trig_ch == ad_ch0 ? F("TG1") : F("TG2")); 
    display.print(trig_edge == TRIG_E_UP ? '/' : '\\'); 
    set_line_color(6);
    display.print(F("Tlev")); 
    set_line_color(7);
    display.print(Start ? F("RUN") : F("HOLD")); 
    break;
  case 1:
    set_line_color(0);
    display.print(F("CH1")); display_ac(CH0DCSW);
    set_line_color(1);
    display_mode(ch0_mode);
    set_line_color(2);
    display_range(range0);
    set_line_color(3);
    display.print(F("OFS1")); 
    set_line_color(4);
    display.print(F("CH2")); display_ac(CH1DCSW);
    set_line_color(5);
    display_mode(ch1_mode);
    set_line_color(6);
    display_range(range1);
    set_line_color(7);
    display.print(F("OFS2"));
    break;
  case 2:
    set_line_color(0);
    display_range(range0);
    set_line_color(1);
    display_rate();
    set_line_color(2);
    if (!fft_mode) {
      display.print(F("FFT")); 
      set_line_color(3);
      display.print(F("FREQ")); 
      set_line_color(4);
      display.print(F("VOLT")); 
      set_line_color(5);
      display.print(F("PWM")); 
      set_line_color(6);
      display.print(F("DUTY")); 
      set_line_color(7);
      display.print(F("FREQ"));
      if (pulse_mode && (item > 20 && item < 24))
        disp_pulse_frq();
    }
    break;
  case 3:
    set_line_color(0);
    display_range(range0);
    set_line_color(1);
    display_rate();
    set_line_color(2);
    display.print(F("DDS"));
    set_line_color(3);
    disp_dds_wave();
    set_line_color(4);
    display.print(F("FREQ"));
    if (dds_mode) disp_dds_freq();
//    set_line_color(5);
//    display.print(F("FCNT"));
//    fcount_disp();
    break;
  }
//  if (info_mode && Start) {
  if (info_mode) {
    dataAnalize();
    if (info_mode & 1)
      measure_frequency();
    if (info_mode & 2)
      measure_voltage();
  }
  DrawText_big();
  if (!full_screen && !fft_mode)
    draw_trig_level(GRIDCOLOR); // draw trig_lv mark
}

void draw_trig_level(int color) { // draw trig_lv mark
  int x, y;

  x = XOFF+DISPLNG+1; y = YOFF+LCD_YMAX - trig_lv;
  display.drawLine(x, y, x+8, y+4, color);
  display.drawLine(x+8, y+4, x+8, y-4, color);
  display.drawLine(x+8, y-4, x, y, color);
}

//unsigned long fcount = 0;
//const double freq_ratio = 20000.0 / 19987.0;

void fcount_disp() {
  if (!fcount_mode) return;
//  if (FreqCount.available()) {
//    fcount = FreqCount.read();
//    fcount = fcount * freq_ratio; // compensate the ceramic osc
//  }
//  display.setTextColor(TXTCOLOR, BGCOLOR); display.setCursor(74, txtLINE6);
//  display.print(fcount); display.print(F("Hz"));
}

void fcount_close() {
  if (!fcount_mode) return;
  fcount_mode = false;
//  FreqCount.end();
}

void display_range(byte rng) {
  display.print(Ranges[rng]);
}

void display_rate(void) {
  display.print(Rates[rate]);
}

void display_mode(byte chmode) {
  display.print(Modes[chmode]); 
}

void display_trig_mode(void) {
  display.print(TRIG_Modes[trig_mode]); 
}

void display_ac(byte pin) {
  if (digitalRead(pin) == LOW) display.print('~');
}

void set_line_color(byte line) {
  if ((item & 0x7) == line) display.setTextColor(BGCOLOR, TXTCOLOR);  // highlight
  else display.setTextColor(TXTCOLOR, BGCOLOR);           // normal
  display.setCursor(DISPTXT, 8 * line); // locate curser for printing text
}

void DrawGrid(int x) {
  if ((x % DOTS_DIV) == 0) {
    for (int y=0; y<=LCD_YMAX; y += 5)
      display.drawPixel(XOFF+x, YOFF+y, GRIDCOLOR);
  } else if ((x % 5) == 0)
    for (int y=0; y<=LCD_YMAX; y += DOTS_DIV)
      display.drawPixel(XOFF+x, YOFF+y, GRIDCOLOR);
}

void ClearAndDrawGraph() {
  int clear;
  byte *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
  int disp_leng;
  if (full_screen) disp_leng = SAMPLES-1;
  else disp_leng = DISPLNG-1;
  bool ch1_active = ch1_mode != MODE_OFF && rate >= RATE_DUAL;
  if (sample == 0)
    clear = 2;
  else
    clear = 0;
  p1 = data[clear+0];
  p2 = p1 + 1;
  p3 = data[sample+0];
  p4 = p3 + 1;
  p5 = data[clear+1];
  p6 = p5 + 1;
  p7 = data[sample+1];
  p8 = p7 + 1;
#if 0
  for (int x=0; x<disp_leng; x++) {
    display.drawPixel(XOFF+x, YOFF+LCD_YMAX-data[sample+0][x], CH1COLOR);
    display.drawPixel(XOFF+x, YOFF+LCD_YMAX-data[sample+1][x], CH2COLOR);
  }
#else
  for (int x=0; x<disp_leng; x++) {
    if (ch0_mode != MODE_OFF) {
      display.drawLine(XOFF+x, YOFF+LCD_YMAX-*p1++, XOFF+x+1, YOFF+LCD_YMAX-*p2++, BGCOLOR);
      display.drawLine(XOFF+x, YOFF+LCD_YMAX-*p3++, XOFF+x+1, YOFF+LCD_YMAX-*p4++, CH1COLOR);
    }
    if (ch1_active) {
      display.drawLine(XOFF+x, YOFF+LCD_YMAX-*p5++, XOFF+x+1, YOFF+LCD_YMAX-*p6++, BGCOLOR);
      display.drawLine(XOFF+x, YOFF+LCD_YMAX-*p7++, XOFF+x+1, YOFF+LCD_YMAX-*p8++, CH2COLOR);
    }
//    CheckSW();
  }
#endif
}

void ClearAndDrawDot(int i) {
#if 0
  for (int x=0; x<DISPLNG; x++) {
    display.drawPixel(XOFF+i, YOFF+LCD_YMAX-odat01, BGCOLOR);
    display.drawPixel(XOFF+i, YOFF+LCD_YMAX-odat11, BGCOLOR);
    display.drawPixel(XOFF+i, YOFF+LCD_YMAX-data[sample+0][i], CH1COLOR);
    display.drawPixel(XOFF+i, YOFF+LCD_YMAX-data[sample+1][i], CH2COLOR);
  }
#else
  if (i < 1) {
    DrawGrid(i);
    return;
  }
  if (ch0_mode != MODE_OFF) {
    display.drawLine(XOFF+i-1, YOFF+LCD_YMAX-odat00,   XOFF+i, YOFF+LCD_YMAX-odat01, BGCOLOR);
    display.drawLine(XOFF+i-1, YOFF+LCD_YMAX-data[0][i-1], XOFF+i, YOFF+LCD_YMAX-data[0][i], CH1COLOR);
  }
  if (ch1_mode != MODE_OFF) {
    display.drawLine(XOFF+i-1, YOFF+LCD_YMAX-odat10,   XOFF+i, YOFF+LCD_YMAX-odat11, BGCOLOR);
    display.drawLine(XOFF+i-1, YOFF+LCD_YMAX-data[1][i-1], XOFF+i, YOFF+LCD_YMAX-data[1][i], CH2COLOR);
  }
#endif
  DrawGrid(i);
}

#define BENDX 3480  // 85% of 4096
#define BENDY 3072  // 75% of 4096

int16_t adc_linearlize(int16_t level) {
  if (level < BENDY) {
    level = map(level, 0, BENDY, 0, BENDX);
  } else {
    level = map(level, BENDY, 4095, BENDX, 4095);
  }
  return level;
}

void scaleDataArray(byte ad_ch, int trig_point)
{
  byte *pdata, ch_mode, range;
  short ch_off;
  uint16_t *idata, *qdata;
  long a, b;

  if (ad_ch == ad_ch1) {
    ch_off = ch1_off;
    ch_mode = ch1_mode;
    range = range1;
    pdata = data[sample+1];
    idata = cap_buf1;
    qdata = payload+SAMPLES;
  } else {
    ch_off = ch0_off;
    ch_mode = ch0_mode;
    range = range0;
    pdata = data[sample+0];
    idata = &cap_buf[trig_point];
    qdata = payload;
  }
  for (int i = 0; i < SAMPLES; i++) {
    *idata = adc_linearlize(*idata);
    a = ((*idata + ch_off) * VREF[range] + 2048) >> 12;
    if (a > LCD_YMAX) a = LCD_YMAX;
    else if (a < 0) a = 0;
    if (ch_mode == MODE_INV)
      a = LCD_YMAX - a;
    *pdata++ = (byte) a;
    b = ((*idata++ + ch_off) * VREF[range] + 101) / 201;
    if (b > 4095) b = 4095;
    else if (b < 0) b = 0;
    if (ch_mode == MODE_INV)
      b = 4095 - b;
    *qdata++ = (int16_t) b;
  }
}

byte adRead(byte ch, byte mode, int off, int i)
{
  int16_t aa = adc1_get_raw((adc1_channel_t) ch);  // ADC read and save approx 46us
  aa = adc_linearlize(aa);
  long a = (((long)aa+off)*VREF[ch == ad_ch0 ? range0 : range1]+2048) >> 12;
  if (a > LCD_YMAX) a = LCD_YMAX;
  else if (a < 0) a = 0;
  if (mode == MODE_INV)
    a = LCD_YMAX - a;
  long b = (((long)aa+off)*VREF[ch == ad_ch0 ? range0 : range1] + 101) / 201;
  if (b > 4095) b = 4095;
  else if (b < 0) b = 0;
  if (mode == MODE_INV)
    b = 4095 - b;
  if (ch == ad_ch1) {
    cap_buf1[i] = aa;
    payload[i+SAMPLES] = b;
  } else {
    cap_buf[i] = aa;
    payload[i] = b;
  }
  return a;
}

int advalue(int value, long vref, byte mode, int offs) {
  if (mode == MODE_INV)
    value = LCD_YMAX - value;
  return ((long)value << 12) / vref - offs;
}

void set_trigger_ad() {
  if (trig_ch == ad_ch0) {
    trigger_ad = advalue(trig_lv, VREF[range0], ch0_mode, ch0_off);
  } else {
    trigger_ad = advalue(trig_lv, VREF[range1], ch1_mode, ch1_off);
  }
}

void loop() {
  int oad, ad;
  unsigned long auto_time;

  timeExec = 100;
//  digitalWrite(LED_BUILTIN, HIGH);  // GPIO2 is used for touch CS
  if (rate > RATE_DMA) {
    set_trigger_ad();
    auto_time = pow(10, rate / 3);
    if (rate < 7)
      auto_time *= 10;
    if (trig_mode != TRIG_SCAN) {
      unsigned long st = millis();
      oad = adc1_get_raw((adc1_channel_t)trig_ch)&0xfffc;
      for (;;) {
        ad = adc1_get_raw((adc1_channel_t)trig_ch)&0xfffc;

        if (trig_edge == TRIG_E_UP) {
          if (ad > trigger_ad && trigger_ad > oad)
            break;
        } else {
          if (ad < trigger_ad && trigger_ad < oad)
            break;
        }
        oad = ad;

        if (rate > 9)
          CheckSW();      // no need for fast sampling
        if (trig_mode == TRIG_SCAN)
          break;
        if (trig_mode == TRIG_AUTO && (millis() - st) > auto_time)
          break; 
      }
    }
  }
  
  // sample and draw depending on the sampling rate
  if (rate < RATE_ROLL && Start) {
    // change the index for the double buffer
    if (sample == 0)
      sample = 2;
    else
      sample = 0;

    if (rate <= RATE_DMA) { // channel 0 only I2S DMA sampling (Max 500ksps)
      sample_i2s(ad_ch0);
    } else if (rate == 6) { // channel 0 only 50us sampling
      sample_200us(50, ad_ch0);
    } else if (rate >= 7 && rate <= 8) {  // dual channel 100us, 200us sampling
      sample_dual_us(HREF[rate] / 10);
    } else {                // dual channel .5ms, 1ms, 2ms, 5ms, 10ms, 20ms sampling
      sample_dual_ms(HREF[rate] / 10);
    }
//    digitalWrite(LED_BUILTIN, LOW); // GPIO2 is used for touch CS
    draw_screen();
  } else if (Start) { // 40ms - 400ms sampling
    timeExec = 5000;
    static const unsigned long r_[] PROGMEM = {40000, 80000, 200000, 400000};
    unsigned long r;
    int disp_leng;
    if (full_screen) disp_leng = SAMPLES;
    else disp_leng = DISPLNG;
//    unsigned long st0 = millis();
    unsigned long st = micros();
    for (int i=0; i<disp_leng; i ++) {
      r = r_[rate - RATE_ROLL];  // rate may be changed in loop
      while((st - micros())<r) {
        CheckSW();
        if (rate<RATE_ROLL)
          break;
      }
      if (rate<RATE_ROLL) { // sampling rate has been changed
        display.fillScreen(BGCOLOR);
        break;
      }
      st += r;
      if (st - micros()>r)
          st = micros(); // sampling rate has been changed to shorter interval
      if (!Start) {
         i --;
         continue;
      }
      odat00 = odat01;      // save next previous data ch0
      odat10 = odat11;      // save next previous data ch1
      odat01 = data[0][i];  // save previous data ch0
      odat11 = data[1][i];  // save previous data ch1
      if (ch0_mode != MODE_OFF) data[0][i] = adRead(ad_ch0, ch0_mode, ch0_off, i);
      if (ch1_mode != MODE_OFF) data[1][i] = adRead(ad_ch1, ch1_mode, ch1_off, i);
      if (ch0_mode == MODE_OFF) payload[0] = -1;
      if (ch1_mode == MODE_OFF) payload[SAMPLES] = -1;
      xTaskNotify(taskHandle, 0, eNoAction);  // notify Websocket server task
      ClearAndDrawDot(i);     
    }
    DrawGrid(disp_leng);  // right side grid   
    // Serial.println(millis()-st0);
//    digitalWrite(LED_BUILTIN, LOW); // GPIO2 is used for touch CS
//    DrawGrid();
    if (!full_screen) DrawText();
  } else {
    DrawText();
  }
  if (trig_mode == TRIG_ONE)
    Start = false;
  CheckSW();
  saveEEPROM();                         // save settings to EEPROM if necessary
}

void draw_screen() {
//  display.fillScreen(BGCOLOR);
  if (fft_mode) {
    DrawText();
    plotFFT();
  } else {
    DrawGrid();
    ClearAndDrawGraph();
    if (!full_screen) DrawText();
    if (ch0_mode == MODE_OFF) payload[0] = -1;
    if (ch1_mode == MODE_OFF) payload[SAMPLES] = -1;
  }
  xTaskNotify(taskHandle, 0, eNoAction);  // notify Websocket server task
  delay(10);    // wait Web task to send it (adhoc fix)
//  display.display();
}

#define textINFO 240
void measure_frequency() {
  int x1, x2;
  freqDuty();
  display.setTextColor(TXTCOLOR, BGCOLOR);
  display.setCursor(textINFO, txtLINE3);
  if (waveFreq < 999.5)
    display.print(waveFreq);
  else if (waveFreq < 999999.5)
    display.print(waveFreq, 0);
  else {
    display.print(waveFreq/1000.0, 0);
    display.print('k');
  }
  display.print(F("Hz"));
  if (fft_mode) return;
  display.setCursor(textINFO + 12, txtLINE4);
  display.print(waveDuty);  display.print('%');
}

void measure_voltage() {
  int x, dave, dmax, dmin;
  if (fft_mode) return;
  if (ch0_mode == MODE_INV) {
    dave = (LCD_YMAX) * 10 - dataAve;
    dmax = dataMin;
    dmin = dataMax;
  } else {
    dave = dataAve;
    dmax = dataMax;
    dmin = dataMin;
  }
  float vavr = VRF * ((dave * 409.6)/ VREF[range0] - ch0_off) / 4096.0;
  float vmax = VRF * advalue(dmax, VREF[range0], ch0_mode, ch0_off) / 4096.0;
  float vmin = VRF * advalue(dmin, VREF[range0], ch0_mode, ch0_off) / 4096.0;
  display.setCursor(textINFO, txtLINE5+24);
  display.print(F("max"));  display.print(vmax); if (vmax >= 0.0) display.print('V');
  display.setCursor(textINFO, txtLINE6+24);
  display.print(F("avr"));  display.print(vavr); if (vavr >= 0.0) display.print('V');
  display.setCursor(textINFO, txtLINE7+24);
  display.print(F("min"));  display.print(vmin); if (vmin >= 0.0) display.print('V');
}

void sample_dual_us(unsigned int r) { // dual channel. r > 67
  if (ch0_mode != MODE_OFF && ch1_mode == MODE_OFF) {
    unsigned long st = micros();
    for (int i=0; i<SAMPLES; i ++) {
      while(micros() - st < r) ;
      cap_buf[i] = adc1_get_raw((adc1_channel_t) ad_ch0);
      st += r;
    }
    scaleDataArray(ad_ch0, 0);
    memset(data[1], 0, SAMPLES);
  } else if (ch0_mode == MODE_OFF && ch1_mode != MODE_OFF) {
    unsigned long st = micros();
    for (int i=0; i<SAMPLES; i ++) {
      while(micros() - st < r) ;
      cap_buf1[i] = adc1_get_raw((adc1_channel_t) ad_ch1);
      st += r;
    }
    scaleDataArray(ad_ch1, 0);
    memset(data[0], 0, SAMPLES);
  } else {
    unsigned long st = micros();
    for (int i=0; i<SAMPLES; i ++) {
      while(micros() - st < r) ;
      cap_buf[i] = adc1_get_raw((adc1_channel_t) ad_ch0);
      cap_buf1[i] = adc1_get_raw((adc1_channel_t) ad_ch1);
      st += r;
    }
    scaleDataArray(ad_ch0, 0);
    scaleDataArray(ad_ch1, 0);
  }
}

void sample_dual_ms(unsigned int r) { // dual channel. r > 500
// .5ms, 1ms or 2ms sampling
  unsigned long st = micros();
  for (int i=0; i<SAMPLES; i ++) {
    while(micros() - st < r) ;
    st += r;
    if (ch0_mode != MODE_OFF) {
      cap_buf[i] = adc1_get_raw((adc1_channel_t) ad_ch0);
    }
    if (ch1_mode != MODE_OFF) {
      cap_buf1[i] = adc1_get_raw((adc1_channel_t) ad_ch1);
    }
  }
//  if (ch0_mode == MODE_OFF) memset(data[0], 0, SAMPLES);
//  if (ch1_mode == MODE_OFF) memset(data[1], 0, SAMPLES);
  scaleDataArray(ad_ch0, 0);
  scaleDataArray(ad_ch1, 0);
}

void sample_200us(unsigned int r, int ad_ch) { // adc1_get_raw() with timing, channel 0 or 1. 1250us/div 20ksps
  uint16_t *idata;
  idata = cap_buf;
  unsigned long st = micros();
//  disableCore1WDT();
  for (int i=0; i<SAMPLES; i ++) {
    while(micros() - st < r) ;
    *idata++ = adc1_get_raw((adc1_channel_t) ad_ch);
    st += r;
//    yield();
//    esp_task_wdt_reset();
  }
//  enableCore1WDT();
  delay(1);
  scaleDataArray(ad_ch, 0);
  delay(1);
}

double vReal[FFT_N]; // Real part array, actually float type
double vImag[FFT_N]; // Imaginary part array

void plotFFT() {
  byte *lastplot, *newplot;
  int ylim = 200;

  int clear = (sample == 0) ? 2 : 0;
  for (int i = 0; i < FFT_N; i++) {
    vReal[i] = cap_buf[i];
    vImag[i] = 0.0;
  }
  FFT.DCRemoval(vReal, FFT_N);
  FFT.Windowing(vReal, FFT_N, FFT_WIN_TYP_HANN, FFT_FORWARD); // Weigh data
  FFT.Compute(vReal, vImag, FFT_N, FFT_FORWARD);          // Compute FFT
  FFT.ComplexToMagnitude(vReal, vImag, FFT_N);            // Compute magnitudes
  newplot = data[sample];
  lastplot = data[clear];
  payload[0] = 0;
  for (int i = 1; i < FFT_N/2; i++) {
    float db = log10(vReal[i]);
    payload[i] = constrain((int)(512.0 * (db + 1.5)), 0, 4095);
    int dat = constrain((int)(25.0 * (db + 1.5)), 0, ylim);
    display.drawFastVLine(i * 2, ylim - lastplot[i], lastplot[i], BGCOLOR); // erase old
    display.drawFastVLine(i * 2, ylim - dat, dat, CH1COLOR);
    newplot[i] = dat;
  }
  draw_scale();
}

void draw_scale() {
  int ylim = 204;
  float fhref, nyquist;
  display.setTextColor(TXTCOLOR);
  display.setCursor(0, ylim); display.print(F("0Hz")); 
  fhref = (float)HREF[rate];
  nyquist = 5.0e6 / fhref; // Nyquist frequency
  long inyquist = nyquist;
  payload[FFT_N/2] = (short) (inyquist / 1000);
  payload[FFT_N/2+1] = (short) (inyquist % 1000);
  if (nyquist > 999.0) {
    nyquist = nyquist / 1000.0;
    if (nyquist > 99.5) {
      display.setCursor(116, ylim); display.print(nyquist/2,0);display.print('k');
      display.setCursor(232, ylim); display.print(nyquist,0);
    } else if (nyquist > 9.95) {
      display.setCursor(122, ylim); display.print(nyquist/2,0);display.print('k');
      display.setCursor(238, ylim); display.print(nyquist,0);
    } else {
      display.setCursor(122, ylim); display.print(nyquist/2,1);display.print('k');
      display.setCursor(232, ylim); display.print(nyquist,1);
    }
    display.print('k');
  } else {
    display.setCursor(116, ylim); display.print(nyquist/2,0);
    display.setCursor(238, ylim); display.print(nyquist,0);
  }
}

#define EEPROM_START 0
void saveEEPROM() {                   // Save the setting value in EEPROM after waiting a while after the button operation.
  int p = EEPROM_START;
  if (saveTimer > 0) {                // If the timer value is positive
    saveTimer = saveTimer - timeExec; // Timer subtraction
    if (saveTimer <= 0) {             // if time up
      EEPROM.write(p++, range0);      // save current status to EEPROM
      EEPROM.write(p++, ch0_mode);
      EEPROM.write(p++, lowByte(ch0_off));  // save as Little endian
      EEPROM.write(p++, highByte(ch0_off));
      EEPROM.write(p++, range1);
      EEPROM.write(p++, ch1_mode);
      EEPROM.write(p++, lowByte(ch1_off));  // save as Little endian
      EEPROM.write(p++, highByte(ch1_off));
      EEPROM.write(p++, rate);
      EEPROM.write(p++, trig_mode);
      EEPROM.write(p++, trig_lv);
      EEPROM.write(p++, trig_edge);
      EEPROM.write(p++, trig_ch);
      EEPROM.write(p++, fft_mode);
      EEPROM.write(p++, info_mode);
      EEPROM.write(p++, item);
      EEPROM.write(p++, pulse_mode);
      EEPROM.write(p++, duty);
      EEPROM.write(p++, p_range);
      EEPROM.write(p++, lowByte(count));  // save as Little endian
      EEPROM.write(p++, highByte(count));
      EEPROM.write(p++, dds_mode);
      EEPROM.write(p++, wave_id);
      EEPROM.write(p++, ifreq & 0xff);
      EEPROM.write(p++, (ifreq >> 8) & 0xff);
      EEPROM.write(p++, (ifreq >> 16) & 0xff);
      EEPROM.write(p++, (ifreq >> 24) & 0xff);
      EEPROM.write(p++, dac_cw_mode);
      EEPROM.commit();    // actually write EEPROM. Necessary for ESP32
    }
  }
}

void set_default() {
  range0 = RANGE_MIN;
  ch0_mode = MODE_ON;
  ch0_off = 0;
  range1 = RANGE_MIN;
  ch1_mode = MODE_ON;
  ch1_off = 2048;
  rate = 6;
  trig_mode = TRIG_AUTO;
  trig_lv = 30;
  trig_edge = TRIG_E_UP;
  trig_ch = ad_ch0;
  fft_mode = false;
  info_mode = 1;  // display frequency and duty.  Voltage display is off
  item = 2;       // menu item
  pulse_mode = false;
  duty = 128;     // PWM 50%
  p_range = 16;   // PWM range
  count = 256;    // PWM 1220Hz
  dds_mode = false;
  wave_id = 0;    // sine wave
  ifreq = 23841;  // 238.41Hz
  dac_cw_mode = false;
}

extern const byte wave_num;

void loadEEPROM() { // Read setting values from EEPROM (abnormal values will be corrected to default)
  int p = EEPROM_START, error = 0;

  range0 = EEPROM.read(p++);                // range0
  if ((range0 < RANGE_MIN) || (range0 > RANGE_MAX)) ++error;
  ch0_mode = EEPROM.read(p++);              // ch0_mode
  if (ch0_mode > 2) ++error;
  *((byte *)&ch0_off) = EEPROM.read(p++);     // ch0_off low
  *((byte *)&ch0_off + 1) = EEPROM.read(p++); // ch0_off high
  if ((ch0_off < -8192) || (ch0_off > 8191)) ++error;

  range1 = EEPROM.read(p++);                // range1
  if ((range1 < RANGE_MIN) || (range1 > RANGE_MAX)) ++error;
  ch1_mode = EEPROM.read(p++);              // ch1_mode
  if (ch1_mode > 2) ++error;
  *((byte *)&ch1_off) = EEPROM.read(p++);     // ch1_off low
  *((byte *)&ch1_off + 1) = EEPROM.read(p++); // ch1_off high
  if ((ch1_off < -8192) || (ch1_off > 8191)) ++error;

  rate = EEPROM.read(p++);                  // rate
  if ((rate < RATE_MIN) || (rate > RATE_MAX)) ++error;
//  if (ch0_mode == MODE_OFF && rate < 5) ++error;  // correct ch0_mode
  trig_mode = EEPROM.read(p++);             // trig_mode
  if (trig_mode > TRIG_SCAN) ++error;
  trig_lv = EEPROM.read(p++);               // trig_lv
  if (trig_lv > LCD_YMAX) ++error;
  trig_edge = EEPROM.read(p++);             // trig_edge
  if (trig_edge > 1) ++error;
  trig_ch = EEPROM.read(p++);               // trig_ch
  if (trig_ch != ad_ch0 && trig_ch != ad_ch1) ++error;
  fft_mode = EEPROM.read(p++);              // fft_mode
  info_mode = EEPROM.read(p++);             // info_mode
  if (info_mode > 3) ++error;
  item = EEPROM.read(p++);                  // item
  if (item > ITEM_MAX) ++error;
  pulse_mode = EEPROM.read(p++);            // pulse_mode
  duty = EEPROM.read(p++);                  // duty
  p_range = EEPROM.read(p++);               // p_range
  if (p_range > 16) ++error;
  *((byte *)&count) = EEPROM.read(p++);     // count low
  *((byte *)&count + 1) = EEPROM.read(p++); // count high
  if (count > 256) ++error;
  dds_mode = EEPROM.read(p++);              // DDS wave id
  wave_id = EEPROM.read(p++);               // DDS wave id
  if (wave_id >= wave_num) ++error;
  *((byte *)&ifreq) = EEPROM.read(p++);     // ifreq low
  *((byte *)&ifreq + 1) = EEPROM.read(p++); // ifreq
  *((byte *)&ifreq + 2) = EEPROM.read(p++); // ifreq
  *((byte *)&ifreq + 3) = EEPROM.read(p++); // ifreq high
  if (ifreq > 99999L) ++error;
  dac_cw_mode = EEPROM.read(p++);              // DDS wave id
  if (error > 0)
    set_default();
}
