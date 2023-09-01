// Sample from the ADC continuously at a particular sample rate
// Copyright (c) 2022, Siliconvalley4066

#include "driver/i2s.h"

#define I2S_NUM                     I2S_NUM_0
#define ADC_UNIT                    ADC_UNIT_1          // ADC1 or ADC2
#define I2S_SAMPLE_RATE             25000
#define I2S_BUFFER_COUNT            4
#define I2S_BUFFER_SIZE             256

void sample_i2s(byte ad_ch) {
  int t;
  size_t bytes_read;

  i2s_read(I2S_NUM, cap_buf, NSAMP * 2, &bytes_read, 20);
  for (int i=0; i < NSAMP/2; i++) {
    cap_buf[i] = cap_buf[i+i] & 0xfff;  // pick up LEFT data and mask MSBs
  }
  delay(1);
  t = trigger_point();
  scaleDataArray(ad_ch, t);
  delay(1);
}

void i2sInit() {
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format       = I2S_CHANNEL_FMT_ALL_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = I2S_BUFFER_COUNT,
    .dma_buf_len          = I2S_BUFFER_SIZE,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_adc_mode(ADC_UNIT, ADC1_CHANNEL_6);
  i2s_adc_enable(I2S_NUM);
}

static const uint32_t sample_rate[6] = {
  500000, // 2us sampling (500ksps)
  250000, // 4us sampling (250ksps)
  100000, // 10us sampling (100ksps)
  75000,  // 13.3us sampling (75ksps)
  50000,  // 20us sampling (50ksps)
  25000}; // 40us sampling (25ksps)

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
  int trigger_ad;
  int i;

  trigger_ad = advalue(trig_lv, VREF[range0], ch0_mode, ch0_off);
  for (i = 0; i < (NSAMP/2 - SAMPLES - 1); ++i) {
    if (trig_edge == TRIG_E_UP) {
      if (cap_buf[i] < trigger_ad && cap_buf[i+1] > trigger_ad)
        break;
    } else {
      if (cap_buf[i] > trigger_ad && cap_buf[i+1] < trigger_ad)
        break;
    }
  }
  return i;
}
