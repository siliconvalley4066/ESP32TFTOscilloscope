/*
   sin(x)/x interpolation for timebase magnification at x2, x5 and x10.
   Copyright (c) 2024, Siliconvalley4066
*/

// line interpolation
//const int16_t line40[40] = {4096, 3686, 3277, 2867, 2458, 2048, 1638, 1229, 819, 410,
//                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
//                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// hann windowed
//const int16_t sinc40[40] = {4096, 4023, 3808, 3467, 3024, 2508, 1954, 1396, 866, 394,
//                            0, -302, -507, -618, -644, -601, -507, -383, -246, -114,
//                            0, 88, 147, 176, 179, 161, 130, 93, 56, 24,
//                            0, -16, -23, -24, -20, -14, -8, -4, -1, 0};

// Blackman windowed
const int16_t sinc40[40] = {4096, 4019, 3793, 3437, 2977, 2447, 1886, 1330, 814, 364,
                            0, -268, -440, -523, -531, -482, -395, -289, -180, -81,
                            0, 58, 93, 106, 104, 90, 70, 48, 28, 11,
                            0, -7, -10, -10, -8, -5, -3, -1, 0, 0};

#define MAGSAMPL 4
#define MAGSTART (MAGSAMPL-1)
#define MAGFORWD (MAGSAMPL+1)

uint16_t magbuf[SAMPLES];

void mag(byte *d, int factor) {  // factor should be 2, 5, or 10
  int s, m, n;
  long sum;
  for (int i = 0; i < SAMPLES; i++) {
    s = (i / factor) + MAGSTART;    // start sample
    m = i % factor;
    if (m == 0) {
      sum = d[s];
    } else {
      sum = 0;
      m = (10 / factor) * m;
      for (n = 0; n < MAGSAMPL; n++) {
        sum += d[s - n] * sinc40[m + 10 * n];
      }
      for (n = 1; n < MAGFORWD; n++) {
        sum += d[s + n] * sinc40[10 * n - m];
      }
      sum /= 4096;
    }
    if (sum > LCD_YMAX) sum = LCD_YMAX;
    else if (sum < 0) sum = 0;
    magbuf[i] = sum;
  }
  for (int i = 0; i < SAMPLES; i++) {
    d[i] = magbuf[i];
  }
}

void mag(uint16_t *d, int factor) {  // factor should be 2, 5, or 10
  int s, m, n;
  long sum;
  for (int i = 0; i < SAMPLES; i++) {
    s = (i / factor) + MAGSTART;    // start sample
    m = i % factor;
    if (m == 0) {
      sum = d[s];
    } else {
      sum = 0;
      m = (10 / factor) * m;
      for (n = 0; n < MAGSAMPL; n++) {
        sum += d[s - n] * sinc40[m + 10 * n];
      }
      for (n = 1; n < MAGFORWD; n++) {
        sum += d[s + n] * sinc40[10 * n - m];
      }
      sum /= 4096;
    }
    if (sum > 4095) sum = 4095;
    else if (sum < 0) sum = 0;
    magbuf[i] = sum;
  }
  for (int i = 0; i < SAMPLES; i++) {
    d[i] = magbuf[i];
  }
}
