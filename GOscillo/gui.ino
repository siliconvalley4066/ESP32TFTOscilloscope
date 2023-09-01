#define DISPLAY_AC

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

void DrawText_small() {
  byte line = 1;

  display.setTextSize(1);

  TextBG(&line, 302, 3);  // HALT
  if (Start == false) {
    display.setTextColor(TFT_RED);
    display.print(F("HLT"));
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
  display.setCursor(x, y);
  if (ch0_mode == MODE_OFF)
    display.setTextColor(TFT_DARKGREY, BGCOLOR);
  else
    display.setTextColor(CH1COLOR, BGCOLOR);
  display.print(F("CH1"));
}

void disp_ch1(int x, int y) {
  display.setCursor(x, y);
//  if (ch1_mode == MODE_OFF || rate < 5 || 16 < rate)
  if (ch1_mode == MODE_OFF || rate < RATE_DUAL)
    display.setTextColor(TFT_DARKGREY, BGCOLOR);
  else
    display.setTextColor(CH2COLOR, BGCOLOR);
  display.print(F("CH2"));
}

void disp_ch0_range() {
  char str[5];
  display.print(Ranges[range0]);
}

void disp_ch1_range() {
  char str[5];
  if (ch1_mode != MODE_OFF) display.setTextColor(TFT_WHITE, BGCOLOR);
  display.print(Ranges[range1]);
}

void disp_sweep_rate() {
  char str[5];
  display.print(Rates[rate]);
}

void disp_trig_edge() {
  display.print(trig_edge == TRIG_E_UP ? '/' : '\\');  // up or down arrow
}

void disp_trig_source() {
  display.print(trig_ch == ad_ch0 ? F("TG1") : F("TG2")); 
}

void disp_trig_mode() {
  char str[5];
  display.print(TRIG_Modes[trig_mode]);
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
  if (ch0_mode == MODE_INV) display.print('-');  // down arrow
  else display.print(' ');
#ifdef DISPLAY_AC
  display_ac_inv(1, CH0DCSW);
#endif
  set_pos_color(60, 1, TFT_WHITE);      // CH1 range
  disp_ch0_range();
  display.setCursor(132, 1);  // Rate
  set_menu_color(SEL_RATE);
  disp_sweep_rate();
  if (fft_mode)
    return;
  display.setCursor(192, 1);  // Trigger level
  set_menu_color(SEL_TGLVL);
  if (16 < rate) {
    display.print(F("EQIV"));
  } else if (menu == SEL_TGLVL) {
    display.print(trig_lv);
  } else {
    display.print(F("TGLV"));
  }
  display.setCursor(252, 1);  // Position/Halt
  if (Start == false) {
    display.setTextColor(TFT_RED, BGCOLOR);
    display.print(F("HALT"));
  } else if (menu == SEL_OFST1) {
    set_menu_color(SEL_OFST1);
    display.print(F("POS1"));
  } else if (menu == SEL_OFST2) {
    set_menu_color(SEL_OFST2);
    display.print(F("POS2"));
  } else {
    display.setTextColor(TFT_DARKGREY, BGCOLOR);
    display.print(F("VPOS"));
  }

  disp_ch1(1, y);         // CH2
  if (ch1_mode == MODE_INV) display.print('-');  // down arrow
  else display.print(' ');
#ifdef DISPLAY_AC
  display_ac_inv(y, CH1DCSW);
#endif
  display.setCursor(60, y);   // CH2 range
  disp_ch1_range();
  set_pos_color(132, y, TFT_WHITE); // Trigger souce
  disp_trig_source(); 
  display.setCursor(192, y);  // Trigger edge
  disp_trig_edge();
  display.setCursor(252, y);  // Trigger mode
  disp_trig_mode();
  display.setTextSize(1);
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
  if (menu == sel) {
    display.setTextColor(TFT_CYAN, BGCOLOR);
  } else {
    display.setTextColor(TFT_WHITE, BGCOLOR);
  }
}
