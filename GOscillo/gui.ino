#define SEL_NONE    0
#define SEL_RANGE1  1
#define SEL_RANGE2  2
#define SEL_RATE    3
#define SEL_TGSRC   4
#define SEL_EDGE    5
#define SEL_TGLVL   6
#define SEL_TGMODE  7
#define SEL_OFST1   8
#define SEL_OFST2   9
#define SEL_FUNC    10
#define SEL_FFT     11
#define SEL_PWM     12
#define SEL_PWMFREQ 13
#define SEL_PWMDUTY 14
#define SEL_DDS     15
#define SEL_DDSWAVE 16
#define SEL_DDSFREQ 17
#define SEL_DISP    18

byte tmenu = 0;

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
      }
    } else if (x < 120) {     // CH1 voltage range
      tmenu = (tmenu != SEL_RANGE1) ? SEL_RANGE1 : SEL_NONE;
    } else if (x < 180) {     // Rate
      tmenu = (tmenu != SEL_RATE) ? SEL_RATE : SEL_NONE;
    } else if (x < 240) {     // Vertical position
      if (tmenu != SEL_OFST1 && tmenu != SEL_OFST2)
        tmenu = SEL_OFST1;
      else if (tmenu == SEL_OFST1)
        tmenu = SEL_OFST2;
      else
        tmenu = SEL_NONE;
    } else if (x < 300) {     // Function
      tmenu = (tmenu != SEL_FUNC) ? SEL_FUNC : SEL_NONE;
    }
  } else if (y > 220) {
    if (tmenu < SEL_FUNC)
      low_touch_base(x);
    else
      low_touch_func(x);
  } else if (y > 30 && y < 210) {
      switch (tmenu) {
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
        if (x < (LCD_WIDTH / 2) - 25) {         // CH1 offset -
          if (ch0_off > -8191)
            ch0_off -= 4096/VREF[range0];
        } else if (x < (LCD_WIDTH / 2) + 25) {  // CH1 offset reset
          if (digitalRead(CH0DCSW) == LOW)    // DC/AC input
            ch0_off = ac_offset[range0];
          else
            ch0_off = 0;
        } else if (x < (LCD_WIDTH - 10)) {      // CH1 offset +
          if (ch0_off < 8191)
            ch0_off += 4096/VREF[range0];
        }
        break;
      case SEL_OFST2:
        if (x < (LCD_WIDTH / 2) - 25) {         // CH2 offset -
          if (ch1_off > -8191)
            ch1_off -= 4096/VREF[range1];
        } else if (x < (LCD_WIDTH / 2) + 25) {  // CH2 offset reset
          if (digitalRead(CH1DCSW) == LOW)    // DC/AC input
            ch1_off = ac_offset[range1];
          else
            ch1_off = 0;
        } else if (x < (LCD_WIDTH - 10)) {      // CH2 offset +
          if (ch1_off < 8191)
            ch1_off += 4096/VREF[range1];
        }
        break;
      case SEL_DDSWAVE:
        if (x < (LCD_WIDTH / 2)) {  // minus
          rotate_wave(false);
        } else {                    // plus
          rotate_wave(true);
        }
        break;
      case SEL_NONE:
        if (x < (LCD_WIDTH - 35))   // avoid halt during trigger level
          Start = !Start;           // halt
        break;
      case SEL_PWMFREQ:
        if (x < (LCD_WIDTH / 2)) {  // minus
          update_frq(1);
        } else {                    // plus
          update_frq(-1);
        }
        break;
      case SEL_PWMDUTY:
        if (x < (LCD_WIDTH / 2)) {  // minus
          if (duty > 1) --duty;
        } else {                    // plus
          if (255 > duty) ++duty;
        }
        break;
      default:                    // do nothing
        if (fft_mode == true) {
          fft_mode = false;
          display.fillScreen(BGCOLOR);
        }
        break;
      }
  }
  if (x > XOFF+DISPLNG && y > YOFF && y <= YOFF+LCD_YMAX) { // trigger level
    draw_trig_level(BGCOLOR); // erase old trig_lv mark
    trig_lv = YOFF+LCD_YMAX - y;
    set_trigger_ad();
  }
  saveTimer = 5000;     // set EEPROM save timer to 5 secnd
}

void low_touch_base(uint16_t x) {
  if (x < 60) {             // CH2 mode
    if (ch1_mode == MODE_ON) {
      ch1_mode = MODE_INV;
    } else if (ch1_mode == MODE_INV) {
      ch1_mode = MODE_OFF;
      display.fillScreen(BGCOLOR);
    } else if (ch1_mode == MODE_OFF) {
      ch1_mode = MODE_ON;
    }
  } else if (x < 120) {     // CH2 voltage range
    tmenu = (tmenu != SEL_RANGE2) ? SEL_RANGE2 : SEL_NONE;
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
  if (tmenu == SEL_FUNC) {
    if (x < 60) {             // FFT
      fft_mode = true;
      display.fillScreen(BGCOLOR);
    } else if (x < 120) {     // PWM
      tmenu = SEL_PWM;
    } else if (x < 180) {     // DDS
      tmenu = SEL_DDS;
    } else if (x < 240) {     // DISP
      tmenu = SEL_DISP;
    } else if (x < 300) {     // Frequency Counter
    }
  } else if (tmenu >= SEL_PWM && tmenu <= SEL_PWMDUTY) {
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
      tmenu = SEL_PWMFREQ;
    } else if (x < 300) {     // Duty
      tmenu = SEL_PWMDUTY;
    }
  } else if (tmenu >= SEL_DDS && tmenu <= SEL_DDSFREQ) {
    if (x < 60) {             // DDS
    } else if (x < 120) {     // ON/OFF
    } else if (x < 180) {     // WAVE
      tmenu = SEL_DDSWAVE;
    } else if (x < 300) {     // Frequency
      tmenu = SEL_DDSFREQ;
    }
  }
}

void DrawText_small() {
  byte line = 1;

  display.setTextSize(1);

  TextBG(&line, 302, 3);  // HALT
  if (Start == false) {
    display.setTextColor(TFT_RED);
    display.print("HLT");
  }

  disp_ch0(302, line);    // CH1
  line += 10;

  TextBG(&line, 296, 4);  // CH1 Range
  disp_ch0_range();

  TextBG(&line, 296, 4);  // CH1 Mode
  if (strlen(Modes[ch0_mode]) < 4) {
    display.setCursor(302, line - 10);
  }
  display.print(Modes[ch0_mode]);

  disp_ch1(302, line);    // CH2
  line += 10;

  TextBG(&line, 296, 4);  // CH2 Range
  disp_ch1_range();

  TextBG(&line, 296, 4);  // CH2 Mode
  if (strlen(Modes[ch1_mode]) < 4) {
    display.setCursor(302, line - 10);
  }
  display.print(Modes[ch1_mode]);

  display.setTextColor(TFT_WHITE);
  TextBG(&line, 296, 4);  // Sweep Rate
  disp_sweep_rate();
  TextBG(&line, 296, 4);  // Trigger Mode
  disp_trig_mode();
  TextBG(&line, 308, 1);  // Trigger Edge
  disp_trig_edge();
  TextBG(&line, 302, 3);  // Trigger Source
  disp_trig_source(); 
  TextBG(&line, 302, 3);  // Trigger Level
  display.print(trig_lv);
}

void disp_ch0(int x, int y) {
  int color = (ch0_mode == MODE_OFF) ? TFT_DARKGREY : CH1COLOR;
  display.setCursor(x, y);
  display.setTextColor(color, BGCOLOR);
  display.print("CH1");
  display.setTextColor(color);
}

void disp_ch1(int x, int y) {
  int color = (ch1_mode == MODE_OFF || rate < RATE_DUAL) ? TFT_DARKGREY : CH2COLOR;
  display.setCursor(x, y);
  display.setTextColor(color, BGCOLOR);
  display.print("CH2");
  display.setTextColor(color);
}

void disp_ch0_range() {
  int color;
  if (tmenu == SEL_RANGE1) {
    color = TFT_CYAN;
  } else if (ch0_mode == MODE_OFF) {
    color = TFT_DARKGREY;
  } else {
    color = TFT_WHITE;
  }
  display.setTextColor(color, BGCOLOR);
  display.print(Ranges[range0]);
}

void disp_ch1_range() {
  int color;
  if (tmenu == SEL_RANGE2) {
    color = TFT_CYAN;
  } else if (ch1_mode == MODE_OFF || rate < RATE_DUAL) {
    color = TFT_DARKGREY;
  } else {
    color = TFT_WHITE;
  }
  display.setTextColor(color, BGCOLOR);
  display.print(Ranges[range1]);
  display.print("  ");
}

void disp_sweep_rate() {
  display.print(Rates[rate]);
}

void disp_trig_edge() {
  display.print(trig_edge == TRIG_E_UP ? "/    " : "\\    "); // up or down arrow
}

void disp_trig_source() {
  display.print(trig_ch == ad_ch0 ? "TG1  " : "TG2  "); 
}

void disp_trig_mode() {
  display.print(TRIG_Modes[trig_mode]);
  display.print(' ');
}

void TextBG(byte *y, int x, byte chrs) {
  int yinc, wid, hi;
  if (info_mode == 1) {
    yinc = 20, wid = 12, hi = 16;
  } else {
    yinc = 10, wid = 6, hi = 8;
  }
  display.fillRect(x, *y, wid * chrs - 1, hi, BGCOLOR);
  display.setCursor(x, *y);
  *y += yinc;
}

//void DrawText() {
//  if (info_mode > 0 && info_mode < 5) {         // 1, 2, 3, 4
//    DrawText_big();
//  } else if (info_mode > 4 && info_mode < 7) {  // 5, 6
//    DrawText_small();
//  } else {                                      // 0 : no text
//    return;
//  }
//  if (info_mode > 0 && (info_mode & 1) > 0 && Start && !fft_mode) {
//    dataAnalize();
//    measure_frequency();
//    measure_voltage();
//  }
//}

#define BOTTOM_LINE 224

void DrawText_big() {
  char str[5];
  byte y;

//  if (info_mode == 1 || info_mode == 2) {
    display.setTextSize(2);
    y = BOTTOM_LINE;
//  } else if (info_mode == 3 || info_mode == 4) {
//    display.setTextSize(1);
//    y = BOTTOM_LINE + 7;
//  } else {
//    return;
//  }
  disp_ch0(1, 1);         // CH1
  display_ac_inv(1, CH0DCSW);
  if (ch0_mode == MODE_INV) display.print('_');  // down arrow
  else display.print(' ');
  display.setCursor(60, 1);   // CH1 range
  disp_ch0_range();
  display.setCursor(132, 1);  // Rate
  set_menu_color(SEL_RATE);
  disp_sweep_rate();
  if (fft_mode)
    return;
//  display.setCursor(192, 1);  // Trigger level
//  set_menu_color(SEL_TGLVL);
//  if (16 < rate) {
//    display.print("EQIV");
//  } else if (tmenu == SEL_TGLVL) {
//    display.print(trig_lv);
//  } else {
//    display.print("TGLV");
//  }
//  display.setCursor(252, 1);  // Position/Halt
  display.setCursor(192, 1);  // Position/Halt
  if (tmenu == SEL_OFST1) {
    set_menu_color(SEL_OFST1);
    display.print("POS1");
  } else if (tmenu == SEL_OFST2) {
    set_menu_color(SEL_OFST2);
    display.print("POS2");
  } else {
    display.setTextColor(TFT_DARKGREY, BGCOLOR);
    display.print("VPOS");
  }
  display.setCursor(252, 1);  // Function
  set_menu_color(SEL_FUNC);
  if (Start == false) {
    display.setTextColor(TFT_RED, BGCOLOR);
    display.print("HALT");
  } else {
    display.print("FUNC");
  }

  if (tmenu >= SEL_DISP) {
    set_pos_color(1, y, TFT_WHITE); // DDS
    display.print("FREQ VOLT  LARG SMAL OFF  ");
  } else if (SEL_DDS <= tmenu && tmenu <= SEL_DDSFREQ) {
    set_pos_color(1, y, TFT_WHITE); // DDS
    display.print("DDS  ON   ");
    set_menu_color(SEL_DDSWAVE);
    display.setCursor(132, y);      // WAVE
    disp_dds_wave();
    display.print(" ");
    set_menu_color(SEL_DDSFREQ);
    disp_dds_freq_btm();
    display.print("   ");
  } else if (SEL_PWM <= tmenu && tmenu <= SEL_PWMDUTY) {
    set_pos_color(1, y, TFT_WHITE); // PWM
    if (pulse_mode == true) {
      display.print("PWM  ON   ");
    } else {
      display.print("PWM  OFF  ");
    }
    set_menu_color(SEL_PWMFREQ);
    disp_pulse_frq_btm();
    set_menu_color(SEL_PWMDUTY);
    disp_pulse_dty();
  } else if (tmenu >= SEL_FUNC) {
    set_pos_color(1, y, TFT_WHITE); // FFT
    display.print("FFT ");
    display.setCursor(60, y);       // PWM
    display.print("PWM   ");
    display.setCursor(132, y);      // DDS
    display.print("DDS  ");
    display.setCursor(192, y);      // DISP
    display.print("DISP ");
    display.setCursor(252, y);      // FCNT
    display.print("FCNT ");
  } else {
    disp_ch1(1, y);         // CH2
    display_ac_inv(y, CH1DCSW);
    if (ch1_mode == MODE_INV) display.print('_');  // down arrow
    else display.print(' ');
    display.setCursor(60, y);   // CH2 range
    disp_ch1_range();
    set_pos_color(132, y, TFT_WHITE); // Trigger souce
    disp_trig_source(); 
    display.setCursor(192, y);  // Trigger edge
    disp_trig_edge();
    display.setCursor(252, y);  // Trigger mode
    disp_trig_mode();
  }
}

void display_ac_inv(byte y, byte sw) {
  byte h, x;
  if (info_mode == 3 || info_mode == 4) {
    h = 7, x = 18;
  } else {
    h = 15, x = 37;
  }
  display.fillRect(x, y, 12, h, BGCOLOR); // clear AC/DC Inv
  if (digitalRead(sw) == LOW) {
    display.print('~');
    if (info_mode == 1 || info_mode == 2) {
      display.setCursor(37, y); // back space
    }
  }
}

void set_pos_color(int x, int y, int color) {
  display.setTextColor(color, BGCOLOR);
  display.setCursor(x, y);
}

void set_menu_color(byte sel) {
  if (tmenu == sel) {
    display.setTextColor(TFT_CYAN, BGCOLOR);
  } else {
    display.setTextColor(TFT_WHITE, BGCOLOR);
  }
}
