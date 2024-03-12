// Sample from the ADC continuously at a particular sample rate
// Copyright (c) 2022, Siliconvalley4066

#include "driver/i2s.h"

#define I2S_NUM                     I2S_NUM_0
#define ADC_UNIT                    ADC_UNIT_1          // ADC1 or ADC2
#define I2S_BUFFER_COUNT            4
#define I2S_BUFFER_SIZE             256

void sample_i2s() {
  byte ch;
  uint16_t *p;
  size_t bytes_read;

  if (ch0_mode == MODE_OFF && ch1_mode != MODE_OFF) {
    ch = ad_ch1;
    p = cap_buf1;
  } else {
    ch = ad_ch0;
    p = cap_buf;
  }
  i2s_set_adc_mode(ADC_UNIT, (adc1_channel_t) ch);

// for I2S_CHANNEL_FMT_ALL_LEFT
//  i2s_read(I2S_NUM, p, NSAMP * 2, &bytes_read, 20);
//  for (int i=0; i < NSAMP/2; i++) {
//    p[i] = p[i+i] & 0xfff;  // pick up LEFT data and mask MSBs
//  }

// for I2S_CHANNEL_FMT_ONLY_LEFT
  i2s_read(I2S_NUM, p, NSAMP, &bytes_read, 20);
// Swap word order to fix ESP32 bug in packing 16bits into 32bits
  int16_t tmp;
  for (int i=0; i < NSAMP/2; i++) {
    if (i&1) {
      p[i] = tmp;
    } else {
      tmp = p[i] & 0xfff;
      p[i] = p[i+1] & 0xfff;
    }
  }
  delay(1);
  scaleDataArray(ch, trigger_point());
  delay(1);
}

static const uint32_t sample_rate[6] = {
  250000, // 4us sampling (250ksps) x10
  250000, // 4us sampling (250ksps) x5
  250000, // 4us sampling (250ksps)
  125000, // 8us sampling (125ksps)
  50000,  // 20us sampling (50ksps)
  25000}; // 40us sampling (25ksps)

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate          = sample_rate[rate],
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = I2S_BUFFER_COUNT,
    .dma_buf_len          = I2S_BUFFER_SIZE,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  if (ch0_mode == MODE_OFF && ch1_mode != MODE_OFF) {
    i2s_set_adc_mode(ADC_UNIT, ADC1_CHANNEL_7);
  } else {
    i2s_set_adc_mode(ADC_UNIT, ADC1_CHANNEL_6);
  }
  i2s_adc_enable(I2S_NUM);
}

void rate_i2s_mode_config(void) {
  if (orate > RATE_DMA && rate <= RATE_DMA) {
    i2sInit();                        // initialize I2S ADC
  } else if (orate <= RATE_DMA && rate > RATE_DMA) {
    i2s_adc_disable(I2S_NUM);
    i2s_driver_uninstall(I2S_NUM);    //stop & destroy i2s driver
    if (dds_mode)
      dac_output_enable(DAC_CHANNEL_1); // fix the problem why?
  }
  if (rate <= RATE_DMA) {
    i2s_set_sample_rates(I2S_NUM, sample_rate[rate]);
  }
}

//void i2s_adc_stop(void) {
//  i2s_stop(I2S_NUM);
//}

int trigger_point() {
  int trigger_ad, i;
  uint16_t *cap;

  if (trig_ch == ad_ch1) {
    trigger_ad = advalue(trig_lv, VREF[range1], ch1_mode, ch1_off);
    cap = cap_buf1;
  } else {
    trigger_ad = advalue(trig_lv, VREF[range0], ch0_mode, ch0_off);
    cap = cap_buf;
  }
  for (i = 0; i < (NSAMP/2 - SAMPLES - 1); ++i) {
    if (trig_edge == TRIG_E_UP) {
      if (cap[i] < trigger_ad && cap[i+1] > trigger_ad)
        break;
    } else {
      if (cap[i] > trigger_ad && cap[i+1] < trigger_ad)
        break;
    }
  }
  return i;
}
