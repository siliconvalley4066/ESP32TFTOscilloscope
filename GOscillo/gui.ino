#define SEL_NONE    0
#define SEL_CH1     1
#define SEL_CH2     2
#define SEL_RANGE1  3
#define SEL_RANGE2  4
#define SEL_RATE    5
#define SEL_TGSRC   6
#define SEL_EDGE    7
#define SEL_TGLVL   8
#define SEL_TGMODE  9
#define SEL_OFST1   10
#define SEL_OFST2   11
#define SEL_FUNC    12
#define SEL_FFT     13
#define SEL_PWM     14
#define SEL_PWMFREQ 15
#define SEL_PWMDUTY 16
#define SEL_DDS     17
#define SEL_DDSWAVE 18
#define SEL_DDSFREQ 19
#define SEL_FCNT    20
#define SEL_DISP    21
#define SEL_DISPFRQ 22
#define SEL_DISPVOL 23
#define SEL_DISPLRG 24
#define SEL_DISPSML 25
#define SEL_DISPOFF 26

#ifndef NOLCD
void CheckTouch() {
  uint16_t x = 0, y = 0; // To store the touch coordinates
  // Pressed will be set true is there is a valid touch on the screen
  bool pressed = display.getTouch(&x, &y);
  if (!pressed) return;
  if (y < 20) {
    if (x < 60) {             // CH1 mode
      if (ch0_mode == MODE_ON) {
        ch0_mode = MODE_INV;
      } else if (ch0_mode == MODE_INV) {
        ch0_mode = MODE_OFF;
        display.fillScreen(BGCOLOR);
      } else if (ch0_mode == MODE_OFF) {
        ch0_mode = MODE_ON;
        display.fillScreen(BGCOLOR);
      }
    } else if (x < 120) {     // CH1 voltage range
      item = (item != SEL_RANGE1) ? SEL_RANGE1 : SEL_NONE;
    } else if (x < 180) {     // Rate
      item = (item != SEL_RATE) ? SEL_RATE : SEL_NONE;
    } else if (x < 240) {     // Vertical position
      if (item != SEL_OFST1 && item != SEL_OFST2)
        item = SEL_OFST1;
      else if (item == SEL_OFST1)
        item = SEL_OFST2;
      else
        item = SEL_NONE;
    } else if (x < 300) {     // Function
      item = (item != SEL_FUNC) ? SEL_FUNC : SEL_NONE;
    }
    clear_bottom_text();    // clear bottom text area
  } else if (y > 220) {
    if (item < SEL_FUNC)
      low_touch_base(x);
    else
      low_touch_func(x);
  } else if ((y > 30 && y < 210) & (x < (LCD_WIDTH - 35))) {  // avoid conflict with trigger level
    switch (item) {
    case SEL_RATE:
      if (x < (LCD_WIDTH / 2)) {  // minus
        updown_rate(7);         // slow
      } else {                    // plus
        updown_rate(3);         // fast
      }
      break;
    case SEL_RANGE1:
      if (x < (LCD_WIDTH / 2)) {  // minus
        updown_ch0range(7);
      } else {                    // plus
        updown_ch0range(3);
      }
      break;
    case SEL_RANGE2:
      if (x < (LCD_WIDTH / 2)) {  // minus
        updown_ch1range(7);
      } else {                    // plus
        updown_ch1range(3);
      }
      break;
    case SEL_TGLVL:
      if (x > (LCD_WIDTH / 2)) {  // trigger level +
        draw_trig_level(BGCOLOR); // erase old trig_lv mark
        if (trig_lv < LCD_YMAX) {
          trig_lv ++;
          set_trigger_ad();
        }
      } else {                    // trigger level -
        draw_trig_level(BGCOLOR); // erase old trig_lv mark
        if (trig_lv > 0) {
          trig_lv --;
          set_trigger_ad();
        }
      }
      break;
    case SEL_OFST1:
      ch0_off = adjust_offset(x, ch0_off, range0, CH0DCSW);
      break;
    case SEL_OFST2:
      ch1_off = adjust_offset(x, ch1_off, range1, CH1DCSW);
      break;
    case SEL_DDSWAVE:
      if (x < (LCD_WIDTH / 2)) {  // minus
        rotate_wave(false);
      } else {                    // plus
        rotate_wave(true);
      }
      break;
    case SEL_DDSFREQ:
      update_ifrq(touch_diff(x));
      break;
    case SEL_NONE:
      Start = !Start;           // halt
      break;
    case SEL_PWMFREQ:
      update_frq(-touch_diff(x));
      break;
    case SEL_PWMDUTY:
      duty = constrain((int)duty + touch_diff(x), 0, 255);
      update_frq(0);
      break;
    default:                    // do nothing
      if (fft_mode == true) {
        wfft = false;
      }
      break;
    }
  }
  if (x > XOFF+DISPLNG && y > YOFF && y <= YOFF+LCD_YMAX) { // trigger level
    draw_trig_level(BGCOLOR); // erase old trig_lv mark
    trig_lv = YOFF+LCD_YMAX - y;
    set_trigger_ad();
  }
  saveTimer = 5000;     // set EEPROM save timer to 5 second
}
#endif

short adjust_offset(uint16_t x, short ch_off, byte range, int dcsw) {
  int val; 
  if (x < (LCD_WIDTH / 2) - 25) {         // CH2 offset -
    if (x < (LCD_WIDTH / 4) - 25)         // CH2 offset -8
      val = ch_off - 32768/VREF[range];
    else
      val = ch_off - 4096/VREF[range];
  } else if (x < (LCD_WIDTH / 2) + 25) {  // CH2 offset reset
    if (digitalRead(dcsw) == LOW)         // DC/AC input
      val = ac_offset[range];
    else
      val = 0;
  } else if (x < (LCD_WIDTH - 10)) {          // CH2 offset +
    if (x > LCD_WIDTH - (LCD_WIDTH / 4) + 25) // CH1 offset +8
      val = ch_off + 32768/VREF[range];
    else
      val = ch_off + 4096/VREF[range];
  }
  return (constrain(val, -8191, 8191));
}

#ifndef NOLCD
int touch_diff(uint16_t x) {
  int diff;
  if (x < (LCD_WIDTH / 8))          diff = -4;
  else if (x < (LCD_WIDTH / 4))     diff = -3;
  else if (x < ((3*LCD_WIDTH) / 8)) diff = -2;
  else if (x < (LCD_WIDTH / 2))     diff = -1;
  else if (x < ((5*LCD_WIDTH) / 8)) diff = 1;
  else if (x < ((3*LCD_WIDTH) / 4)) diff = 2;
  else if (x < ((7*LCD_WIDTH) / 8)) diff = 3;
  else diff = 4;
  return (diff);
}

void low_touch_base(uint16_t x) {
  if (x < 60) {             // CH2 mode
    if (rate < RATE_DUAL && ch0_mode != MODE_OFF) {
      ch0_mode = MODE_OFF;
      ch1_mode = MODE_ON;
      display.fillScreen(BGCOLOR);
    } else if (ch1_mode == MODE_ON) {
      ch1_mode = MODE_INV;
    } else if (ch1_mode == MODE_INV) {
      ch1_mode = MODE_OFF;
      display.fillScreen(BGCOLOR);
    } else if (ch1_mode == MODE_OFF) {
      ch1_mode = MODE_ON;
    }
  } else if (x < 120) {     // CH2 voltage range
    item = (item != SEL_RANGE2) ? SEL_RANGE2 : SEL_NONE;
  } else if (x < 180) {     // Trigger source
    if (trig_ch == ad_ch0)
      trig_ch = ad_ch1;
    else
      trig_ch = ad_ch0;
    set_trigger_ad();
  } else if (x < 240) {     // Trigger edge
    if (trig_edge == TRIG_E_UP)
      trig_edge = TRIG_E_DN;
    else
      trig_edge = TRIG_E_UP;
  } else if (x < 300) {     // Trigger mode
    if (trig_mode < TRIG_ONE)
      trig_mode ++;
    else
      trig_mode = 0;
    if (trig_mode != TRIG_ONE)
      Start = true;
  }
}

void low_touch_func(uint16_t x) {
  if (item == SEL_FUNC) {
    if (x < 60) {             // FFT
      wfft = true;
    } else if (x < 120) {     // PWM
      item = SEL_PWM;
      clear_bottom_text();                          // clear bottom text area
    } else if (x < 180) {     // DDS
      item = SEL_DDS;
      clear_bottom_text();                          // clear bottom text area
    } else if (x < 240) {     // DISP
      item = SEL_DISP;
      clear_bottom_text();                          // clear bottom text area
    } else if (x < 300) {     // Frequency Counter
    }
  } else if (item >= SEL_PWM && item <= SEL_PWMDUTY) {
    if (x < 60) {             // PWM
    } else if (x < 120) {     // ON/OFF
      if (pulse_mode == false) {  // turn on
        update_frq(0);
        pulse_start();
        pulse_mode = true;
      } else {                    // turn off
        pulse_close();
        pulse_mode = false;
      }
    } else if (x < 240) {     // Frequency
      item = SEL_PWMFREQ;
    } else if (x < 300) {     // Duty
      item = SEL_PWMDUTY;
    }
  } else if (item >= SEL_DDS && item <= SEL_DDSFREQ) {
    if (x < 60) {               // DDS
    } else if (x < 120) {     // ON/OFF
      if (dds_mode == false) {  // turn on
        dds_setup();
        dds_mode = wdds = true;
      } else {                  // turn off
        dds_close();
        dds_mode = wdds = false;
      }
    } else if (x < 180) {     // WAVE
      item = SEL_DDSWAVE;
    } else if (x < 300) {     // Frequency
      item = SEL_DDSFREQ;
    }
  } else if (item >= SEL_DISP && item <= SEL_DISPOFF) {
    if (x < 60) {         // SEL_DISPFRQ
      info_mode = (info_mode & 0x3c) | ((info_mode + 1) & 0x3);
      clear_big_text();
    } else if (x < 120) { // SEL_DISPVOL
      info_mode = (info_mode & 0x33) | ((info_mode + 4) & 0xc);
      clear_big_text();
    } else if (x < 180) { // SEL_DISPLRG;
      info_mode |= INFO_BIG;
      clear_text();
    } else if (x < 240) { // SEL_DISPSML;
      info_mode &= ~INFO_BIG;
      clear_text();
    } else if (x < 300) { // SEL_DISPOFF;
      info_mode ^= INFO_OFF;
      clear_text();
    }
  }
}

void disp_ch0(int x, int y) {
  int color = (ch0_mode == MODE_OFF) ? OFFCOLOR : CH1COLOR;
  int bg = (item == SEL_CH1) ? HIGHCOLOR : BGCOLOR;
  display.setCursor(x, y);
  display.setTextColor(color, bg);
  display.print("CH1");
  display.setTextColor(color);
}

void disp_ch1(int x, int y) {
  int color = (ch1_mode == MODE_OFF || (rate < RATE_DUAL && ch0_mode != MODE_OFF)) ? OFFCOLOR : CH2COLOR;
  int bg = (item == SEL_CH2) ? HIGHCOLOR : BGCOLOR;
  display.setCursor(x, y);
  display.setTextColor(color, bg);
  display.print("CH2");
  display.setTextColor(color);
}

void disp_ch0_range() {
  int color;
  if (item == SEL_RANGE1) {
    color = HIGHCOLOR;
  } else if (ch0_mode == MODE_OFF) {
    color = OFFCOLOR;
  } else {
    color = TXTCOLOR;
  }
  display.setTextColor(color, BGCOLOR);
  display.print(Ranges[range0]);
}

void disp_ch1_range() {
  int color;
  if (item == SEL_RANGE2) {
    color = HIGHCOLOR;
  } else if (ch1_mode == MODE_OFF || rate < RATE_DUAL) {
    color = OFFCOLOR;
  } else {
    color = TXTCOLOR;
  }
  display.setTextColor(color, BGCOLOR);
  display.print(Ranges[range1]);
  display.print("  ");
}

void disp_sweep_rate() {
  set_menu_color(SEL_RATE);
  display.print(Rates[rate]);
}

void disp_trig_edge() {
  set_menu_color(SEL_EDGE);
  display.print(trig_edge == TRIG_E_UP ? "/    " : "\\    "); // up or down arrow
}

void disp_trig_source() {
  set_menu_color(SEL_TGSRC);
  display.print(trig_ch == ad_ch0 ? "TG1  " : "TG2  "); 
}

void disp_trig_mode() {
  set_menu_color(SEL_TGMODE);
  display.print(TRIG_Modes[trig_mode]);
  display.print(' ');
}

void TextBG(byte *y, int x, byte chrs) {
  int yinc, wid, hi;
  if (info_mode & INFO_BIG) {
    yinc = 20, wid = 12, hi = 16;
  } else {
    yinc = 10, wid = 6, hi = 8;
  }
  display.fillRect(x, *y, wid * chrs - 1, hi, BGCOLOR);
  display.setCursor(x, *y);
  *y += yinc;
}

#define BOTTOM_LINE 224

void DrawText_big() {
  char str[5];
  byte y;

  if (!(info_mode & INFO_OFF)) {
    if (info_mode & INFO_BIG) {
      display.setTextSize(2);
      y = BOTTOM_LINE;
    } else {
      display.setTextSize(1);
      y = BOTTOM_LINE + 7;
    }
  } else {
    return;
  }
  disp_ch0(1, 1);         // CH1
  display_ac_inv(1, CH0DCSW, ch0_mode);
  display.setCursor(60, 1);   // CH1 range
  disp_ch0_range();
  display.setCursor(132, 1);  // Rate
  disp_sweep_rate();
  if (fft_mode)
    return;
  display.setCursor(192, 1);  // Position Offset
  if (item == SEL_OFST1) {
    set_menu_color(SEL_OFST1);
    display.print("POS1");
  } else if (item == SEL_OFST2) {
    set_menu_color(SEL_OFST2);
    display.print("POS2");
  } else if (item == SEL_TGLVL) {
    set_menu_color(SEL_TGLVL);
    display.print("TGLV");
  } else {
    display.setTextColor(OFFCOLOR, BGCOLOR);
    display.print("VPOS");
  }
  display.setCursor(252, 1);  // Function
  set_menu_color(SEL_FUNC);
  if (Start == false) {
    display.setTextColor(REDCOLOR, BGCOLOR);
    display.print("HALT");
  } else {
    display.print("FUNC");
  }

  if (item >= SEL_DISP) {
//    display.print("FREQ VOLT  LARG SMAL OFF  ");
    set_pos_menu(1, y, SEL_DISPFRQ);    // Frequency
    display.print("FREQ ");
    set_pos_menu(60, y, SEL_DISPVOL);   // Voltage
    display.print("VOLT  ");
    set_pos_menu(132, y, SEL_DISPLRG);  // Large
    display.print("LARG ");
    set_pos_menu(192, y, SEL_DISPSML);  // Small
    display.print("SMAL ");
    set_pos_menu(252, y, SEL_DISPOFF);  // Off
    display.print("OFF  ");
  } else if (SEL_DDS <= item && item <= SEL_DDSFREQ) {
    set_pos_color(1, y, TXTCOLOR);     // DDS
    display.print("DDS  ");
    set_pos_menu(60, y, SEL_DDS);       // ON/OFF
    if (dds_mode == true) {
      display.print("ON   ");
    } else {
      display.print("OFF  ");
    }
    set_menu_color(SEL_DDSWAVE);
    set_pos_menu(132, y, SEL_DDSWAVE);  // WAVE
    disp_dds_wave();
    display.print(" ");
    set_pos_menu(192, y, SEL_DDSFREQ);  // Frequency
    disp_dds_freq();
    display.print("   ");
  } else if (SEL_PWM <= item && item <= SEL_PWMDUTY) {
    set_pos_color(1, y, TXTCOLOR); // PWM
    display.print("PWM  ");
    set_pos_menu(60, y, SEL_PWM);       // ON/OFF
    if (pulse_mode == true) {
      display.print("ON   ");
    } else {
      display.print("OFF  ");
    }
    set_pos_menu(108, y, SEL_PWMFREQ);  // Frequency
    disp_pulse_frq();
    set_pos_menu(240, y, SEL_PWMDUTY);  // Duty
    disp_pulse_dty();
  } else if (item >= SEL_FUNC) {
    set_pos_menu(1, y, SEL_FFT);    // FFT
    display.print("FFT ");
    set_pos_menu(60, y, SEL_PWM);   // PWM
    display.print("PWM   ");
    set_pos_menu(132, y, SEL_DDS);  // DDS
    display.print("DDS  ");
    set_pos_menu(192, y, SEL_DISP); // DISP
    display.print("DISP ");
    set_pos_menu(252, y, SEL_FCNT); // FCNT
    display.print("     ");
  } else {
    disp_ch1(1, y);         // CH2
    display_ac_inv(y, CH1DCSW, ch1_mode);
    display.setCursor(60, y);   // CH2 range
    disp_ch1_range();
    set_pos_color(132, y, TXTCOLOR); // Trigger souce
    disp_trig_source(); 
    display.setCursor(192, y);  // Trigger edge
    disp_trig_edge();
    display.setCursor(252, y);  // Trigger mode
    disp_trig_mode();
  }
}

void display_ac_inv(byte y, byte sw, byte ch_mode) {
  byte h, x;
  if (info_mode & INFO_BIG) {
    h = 15, x = 37;     // big font
  } else {
    h = 7, x = 18;
  }
  display.fillRect(x, y, 12, h, BGCOLOR); // clear AC/DC Inv
  if (digitalRead(sw) == LOW) {
    display.print('~');
    if (info_mode & INFO_BIG) {        // big font
      display.setCursor(37, y); // back space
    }
  }
  if (ch_mode == MODE_INV) display.print('_');  // down arrow
  else display.print(' ');
}

void set_pos_color(int x, int y, int color) {
  display.setTextColor(color, BGCOLOR);
  display.setCursor(x, y);
}

void set_menu_color(byte sel) {
  if (item == sel) {
    display.setTextColor(HIGHCOLOR, BGCOLOR);
  } else {
    display.setTextColor(TXTCOLOR, BGCOLOR);
  }
}

void set_pos_menu(int x, int y, byte sel) {
  display.setCursor(x, y);
  if (item == sel) {
    display.setTextColor(HIGHCOLOR, BGCOLOR);
  } else {
    display.setTextColor(TXTCOLOR, BGCOLOR);
  }
}
#endif

#define BTN_UP    0
#define BTN_DOWN  10
#define BTN_LEFT  7
#define BTN_RIGHT 3
#define BTN_FULL  12
#define BTN_RESET 11

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

#ifndef NOLCD
  CheckTouch();
#endif
  if (wrate != 0) {
    updown_rate(wrate);
    wrate = 0;
    saveTimer = 5000;     // set EEPROM save timer to 5 second
  }

#ifndef NOLCD
#ifdef BUTTON5DIR
  if (digitalRead(DOWNPIN) == LOW && digitalRead(LEFTPIN) == LOW) {
    sw = BTN_RESET; // both button press
  } else if (digitalRead(UPPIN) == LOW && digitalRead(RIGHTPIN) == LOW) {
    sw = BTN_FULL;  // both button press
#else
  if (digitalRead(RIGHTPIN) == LOW && digitalRead(LEFTPIN) == LOW) {
    sw = BTN_RESET; // both button press
  } else if (digitalRead(UPPIN) == LOW && digitalRead(DOWNPIN) == LOW) {
    sw = BTN_FULL;  // both button press
#endif
  } else if (digitalRead(DOWNPIN) == LOW) {
    sw = BTN_DOWN;  // down
  } else if (digitalRead(RIGHTPIN) == LOW) {
    sw = BTN_RIGHT; // right
  } else if (digitalRead(LEFTPIN) == LOW) {
    sw = BTN_LEFT;  // left
  } else if (digitalRead(UPPIN) == LOW) {
    sw = BTN_UP;    // up
  } else {
    lastsw = 255;
    return;
  }
  if (sw != lastsw)
    vtime = ms;
  saveTimer = 5000;     // set EEPROM save timer to 5 second
  menu_sw(sw); 
  DrawText();
//  display.display();
  lastsw = sw;
#endif
}

void updown_ch0range(byte sw) {
  if (sw == BTN_RIGHT) {        // CH0 RANGE +
    if (range0 > 0)
      range0 --;
  } else if (sw == BTN_LEFT) {  // CH0 RANGE -
    if (range0 < RANGE_MAX)
      range0 ++;
  }
}

void updown_ch1range(byte sw) {
  if (sw == BTN_RIGHT) {        // CH1 RANGE +
    if (range1 > 0)
      range1 --;
  } else if (sw == BTN_LEFT) {  // CH1 RANGE -
    if (range1 < RANGE_MAX)
      range1 ++;
  }
}

void updown_rate(byte sw) {
  if (sw == BTN_RIGHT) {        // RATE FAST
    orate = rate;
    if (rate > 0) {
      rate --;
      rate_i2s_mode_config();
    }
#ifndef NOLCD
    display.fillScreen(BGCOLOR);
#endif
  } else if (sw == BTN_LEFT) {  // RATE SLOW
    orate = rate;
    if (rate < RATE_MAX) {
      rate ++;
      rate_i2s_mode_config();
    } else {
      rate = RATE_MAX;
    }
  }
}

#ifndef NOLCD
void menu_sw(byte sw) {  
  int diff;
  switch (item) {
  case SEL_CH1:   // CH0 mode
    if (sw == BTN_RIGHT) {        // CH0 + ON/INV
      if (ch0_mode == MODE_ON)
        ch0_mode = MODE_INV;
      else
        ch0_mode = MODE_ON;
    } else if (sw == BTN_LEFT) {  // CH0 - ON/OFF
      if (ch0_mode == MODE_OFF)
        ch0_mode = MODE_ON;
      else {
        ch0_mode = MODE_OFF;
        display.fillScreen(BGCOLOR);
      }
    }
    break;
  case SEL_CH2:   // CH1 mode
    if (sw == BTN_RIGHT) {        // CH1 + ON/INV
      if (ch1_mode == MODE_ON)
        ch1_mode = MODE_INV;
      else
        ch1_mode = MODE_ON;
    } else if (sw == BTN_LEFT) {  // CH1 - ON/OFF
      if (rate < RATE_DUAL) {
        ch0_mode = MODE_OFF;
        ch1_mode = MODE_ON;
        display.fillScreen(BGCOLOR);
      } else if (ch1_mode == MODE_OFF)
        ch1_mode = MODE_ON;
      else {
        ch1_mode = MODE_OFF;
        display.fillScreen(BGCOLOR);
      }
    }
    break;
  case SEL_RANGE1:  // CH0 voltage range
    updown_ch0range(sw);
    break;
  case SEL_RANGE2:  // CH1 voltage range
    updown_ch1range(sw);
    break;
  case SEL_RATE:    // rate
    updown_rate(sw);
    break;
  case SEL_TGMODE:  // trigger mode
    if (sw == BTN_RIGHT) {        // TRIG MODE +
      if (trig_mode < TRIG_ONE)
        trig_mode ++;
      else
        trig_mode = 0;
    } else if (sw == BTN_LEFT) {  // TRIG MODE -
      if (trig_mode > 0)
        trig_mode --;
      else
        trig_mode = TRIG_ONE;
    }
    if (trig_mode != TRIG_ONE)
        Start = true;
    break;
  case SEL_TGSRC:   // trigger source
    if (sw == BTN_RIGHT) {
      trig_ch = ad_ch1;
    } else if (sw == BTN_LEFT) {
      trig_ch = ad_ch0;
    }
    set_trigger_ad();
    break;
  case SEL_EDGE:    // trigger polarity
    if (sw == BTN_RIGHT) {        // trigger + edge
      trig_edge = TRIG_E_DN;
    } else if (sw == BTN_LEFT) {  // trigger - channel
      trig_edge = TRIG_E_UP;
    }
    break;
  case SEL_TGLVL:   // trigger level
    if (sw == BTN_RIGHT) {        // trigger level +
      draw_trig_level(BGCOLOR);   // erase old trig_lv mark
      if (trig_lv < LCD_YMAX) {
        trig_lv ++;
        set_trigger_ad();
      }
    } else if (sw == BTN_LEFT) {  // trigger level -
      draw_trig_level(BGCOLOR);   // erase old trig_lv mark
      if (trig_lv > 0) {
        trig_lv --;
        set_trigger_ad();
      }
    }
    break;
  case SEL_OFST1: // CH0 offset
    if (sw == BTN_RIGHT) {        // offset +
      if (ch0_off < 8191)
        ch0_off += 4096/VREF[range0];
    } else if (sw == BTN_LEFT) {  // offset -
      if (ch0_off > -8191)
        ch0_off -= 4096/VREF[range0];
    } else if (sw == BTN_RESET) { // offset reset
      if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
        ch0_off = ac_offset[range0];
      else
        ch0_off = 0;
    }
    break;
  case SEL_OFST2: // CH1 offset
    if (sw == BTN_RIGHT) {        // offset +
      if (ch1_off < 8191)
        ch1_off += 4096/VREF[range1];
    } else if (sw == BTN_LEFT) {  // offset -
      if (ch1_off > -8191)
        ch1_off -= 4096/VREF[range1];
    } else if (sw == BTN_RESET) { // offset reset
      if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
        ch1_off = ac_offset[range1];
      else
        ch1_off = 0;
    }
    break;
  case SEL_FFT:   // FFT mode
    if (sw == BTN_RIGHT) {        // ON
      wfft = true;
    } else if (sw == BTN_LEFT) {  // OFF
      wfft = false;
    }
    break;
  case SEL_PWM: // PWM
    if (sw == BTN_RIGHT) {        // +
      update_frq(0);
      pulse_start();
      pulse_mode = true;
    } else if (sw == BTN_LEFT) {  // -
      pulse_close();
      pulse_mode = false;
    }
    break;
  case SEL_PWMDUTY: // PWM Duty ratio
    diff = 1;
    if (sw == lastsw) {
      if (millis() - vtime > 5000) diff = 8;
    }
    if (sw == BTN_RIGHT) {        // +
      if (pulse_mode) {
        if ((256 - duty) > diff) duty += diff;
      } else {
        pulse_start();
      }
      update_frq(0);
      pulse_mode = true;
    } else if (sw == BTN_LEFT) {  // -
      if (pulse_mode) {
        if (duty > diff) duty -= diff;
      } else {
        pulse_start();
      }
      update_frq(0);
      pulse_mode = true;
    }
    break;
  case SEL_PWMFREQ: // PWM Frequency
    diff = sw_accel(sw);
    if (sw == BTN_RIGHT) {        // +
      if (pulse_mode)
        update_frq(-diff);
      else {
        update_frq(0);
        pulse_start();
      }
      pulse_mode = true;
    } else if (sw == BTN_LEFT) {  // -
      if (pulse_mode)
        update_frq(diff);
      else {
        update_frq(0);
        pulse_start();
      }
      pulse_mode = true;
    }
    break;
  case SEL_DDS:     // DDS
    if (sw == BTN_RIGHT) {        // +
      dds_setup();
      dds_mode = wdds = true;
    } else if (sw == BTN_LEFT) {  // -
      dds_close();
      dds_mode = wdds = false;
    }
    break;
  case SEL_DDSWAVE: // WAVE
    if (sw == BTN_RIGHT) {        // +
      rotate_wave(true);
    } else if (sw == BTN_LEFT) {  // -
      rotate_wave(false);
    }
    break;
  case SEL_DDSFREQ: // FREQ
    diff = sw_accel(sw);
    if (sw == BTN_RIGHT) {        // +
      update_ifrq(diff);
    } else if (sw == BTN_LEFT) {  // -
      update_ifrq(-diff);
    }
    break;
  case SEL_DISPFRQ: // Frequency and Duty display
    if (sw == BTN_RIGHT) {        // ON
      info_mode = (info_mode & 0x3c) | ((info_mode + 1) & 0x3);
      clear_big_text();                             // clear big text area
    } else if (sw == BTN_LEFT) {  // OFF
      info_mode = (info_mode & 0x3c) | ((info_mode - 1) & 0x3);
      clear_big_text();                             // clear big text area
    }
    break;
  case SEL_DISPVOL: // Voltage display
    if (sw == BTN_RIGHT) {        // ON
      info_mode = (info_mode & 0x33) | ((info_mode + 4) & 0xc);
      clear_big_text();                             // clear big text area
    } else if (sw == BTN_LEFT) {  // OFF
      info_mode = (info_mode & 0x33) | ((info_mode - 4) & 0xc);
      clear_big_text();                             // clear big text area
    }
    break;
  case SEL_DISPLRG: // Large Font
    if (sw == BTN_RIGHT) {        // ON
      info_mode |= INFO_BIG;
      clear_big_text();                             // clear big text area
    } else if (sw == BTN_LEFT) {  // OFF
      info_mode &= ~INFO_BIG;
      clear_text();
    }
    break;
  case SEL_DISPSML: // Small Font
    if (sw == BTN_RIGHT) {        // ON
      info_mode &= ~INFO_BIG;
      clear_text();
    } else if (sw == BTN_LEFT) {  // OFF
      info_mode |= INFO_BIG;
      clear_big_text();                             // clear big text area
    }
    break;
  case SEL_DISPOFF: // Text Display Off
    if (sw == BTN_RIGHT) {        // OFF
      info_mode |= INFO_OFF;
      clear_text();
    } else if (sw == BTN_LEFT) {  // ON
      info_mode &= ~INFO_OFF;
    }
    break;
  }
  menu_updown(sw);
}

void clear_big_text() {
  display.fillRect(214, 22, 96, 200, BGCOLOR);  // clear big text area
}

void clear_text() {
  clear_big_text();                             // clear big text area
  display.fillRect(0, 0, 319, 20, BGCOLOR);     // clear top text area
  clear_bottom_text();                          // clear bottom text area
}

void clear_bottom_text() {
  display.fillRect(0, 221, 319, 19, BGCOLOR);   // clear bottom text area
}

void menu_updown(byte sw) {
  if (sw == BTN_DOWN) {       // MENU down SW
    increment_item();
  } else if (sw == BTN_UP) {  // Menu up SW
    decrement_item();
  }
}

void increment_item() {
  ++item;
  if (item > SEL_DISPOFF) item = 0;
  if (item != SEL_FFT) wfft = false;      // exit FFT mode
  if ((item == SEL_PWM) || (item == SEL_DDS) || (item == SEL_DISP))
    clear_bottom_text();                  // clear bottom text area
}

void decrement_item() {
  if (item > 0) --item;
  else item = SEL_DISPOFF;
  if (item != SEL_FFT) wfft = false;      // exit FFT mode
  if ((item == SEL_FFT) || (item == SEL_PWM) || (item == SEL_DDSFREQ)
    || (item == SEL_PWMDUTY) || (item == SEL_DISPOFF))
    clear_bottom_text();                  // clear bottom text area
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
#endif
