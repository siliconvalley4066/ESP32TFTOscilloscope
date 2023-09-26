#define LEDC_CHANNEL_0 0 //channel max 15
#define GPIO_PIN 16 // should not assign GPIO #36 throough #39

byte duty = 128;      // duty ratio = duty/256
byte p_range = 16;    // bit_num 1 - 16
unsigned short count;   // rate 256/256 - 1/256

double pulse_frq(void) {      // 4.768Hz <= pulse_frq <= 40MHz
  return(80.0e6 / pow(2, p_range) * count / 256.0);
}

void set_pulse_frq(float freq) {  // 4.768Hz <= freq <= 40MHz
  if (freq > 40e6) freq = 40e6;
  p_range = constrain(int(log(80e6/freq)/log(2)), 1, 16);
  count = round(256.0 / 80.0e6 * pow(2, p_range) * freq);
  ledcSetup(LEDC_CHANNEL_0, pulse_frq(), p_range);
  setduty();
}

void pulse_init() {
  int divide;
  p_range = constrain(p_range, 1, 16);
  if (p_range < 16)
    count = constrain(count, 129, 256);
  else
    count = constrain(count, 1, 256);
  pinMode(GPIO_PIN, OUTPUT);
  ledcSetup(LEDC_CHANNEL_0, pulse_frq(), p_range);
  ledcAttachPin(GPIO_PIN, LEDC_CHANNEL_0);
  setduty();
//  ledcWrite(LEDC_CHANNEL_0, ((long)duty * (long)pow(2, p_range)) >> 8);
}

void update_frq(int diff) {
  int divide, fast;
  long newCount;

  if (abs(diff) > 3) {
    fast = 8;
  } else if (abs(diff) > 2) {
    fast = 4;
  } else if (abs(diff) > 1) {
    fast = 2;
  } else {
    fast = 1;
  }
  newCount = (long)count - fast * diff;

  if (newCount < 129) {
    if (p_range > 15) {
      newCount = constrain(newCount, 0, 256);
    } else {
      ++p_range;
      newCount = 256;
    }
  } else if (newCount > 256) {
    if (p_range > 1) {
      --p_range;
      newCount = 129;
    } else {
      newCount = 256;
    }
  }
  count = newCount;
  // set TOP value
  ledcSetup(LEDC_CHANNEL_0, pulse_frq(), p_range);
  // set Duty ratio
  setduty();
//  ledcWrite(LEDC_CHANNEL_0, ((long)duty * (long)pow(2, p_range)) >> 8);
}

#ifndef NOLCD
void disp_pulse_frq(void) {
//  freq = ledcReadFreq(LEDC_CHANNEL_0);
  double freq = pulse_frq();
  if (freq < 10000000.0) {
    display.print(" ");
  }
  if (freq < 10.0) {
    display.print(freq, 5);
  } else if (freq < 100.0) {
    display.print(freq, 4);
  } else if (freq < 1000.0) {
    display.print(freq, 3);
  } else if (freq < 10000.0) {
    display.print(freq, 2);
  } else if (freq < 100000.0) {
    display.print(freq, 1);
  } else if (freq < 1000000.0) {
    display.print(" ");
    display.print(freq, 0);
  } else if (freq < 10000000.0) {
    display.print(freq, 0);
  } else {
    display.print(freq, 0);
  }
  display.print("Hz ");
  if ((duty*100.0/256.0) < 9.95) {
    display.print(" ");
  }
}

void disp_pulse_dty(void) {
//  display.print(duty*100.0/256.0, 1); display.print('%');
  display.print(duty*0.390625, 1); display.print("%  ");
}
#endif

void pulse_start() {
  pinMode(GPIO_PIN, OUTPUT);
  ledcSetup(LEDC_CHANNEL_0, pulse_frq(), p_range);
  ledcAttachPin(GPIO_PIN, LEDC_CHANNEL_0);
  setduty();
}

void pulse_close() {
  ledcDetachPin(GPIO_PIN);
  pinMode(GPIO_PIN, INPUT_PULLUP);
}

void setduty(void) {
  if (p_range == 1)
    ledcWrite(LEDC_CHANNEL_0, 0);   // duty=1 will not work
  else
    ledcWrite(LEDC_CHANNEL_0, ((long)duty * (long)pow(2, p_range)) >> 8);
}
