// Sample from the ADC continuously at a particular sample rate
// Copyright (c) 2026, Siliconvalley4066

#include <driver/adc.h>
#include <esp_adc/adc_continuous.h>

adc_continuous_handle_t adc_handle = NULL;
#ifndef ESP32_C3
static adc_channel_t channel[2] = { ADC_CHANNEL_6, ADC_CHANNEL_7 };
#else
static adc_channel_t channel[2] = { ADC_CHANNEL_1, ADC_CHANNEL_3 };
#endif

#ifndef ESP32_C3
void sample_dma(void) {
  adc_digi_output_data_t *data = (adc_digi_output_data_t *)dma_buf;
  uint32_t num_bytes_read = 0;

  adc_continuous_read(adc_handle, (uint8_t *)dma_buf, sizeof(dma_buf),
                      &num_bytes_read, 100);  // 100ms timeout
  int num_samples = num_bytes_read / sizeof(adc_digi_output_data_t);
  // Serial.println(num_samples);

  int i1 = 0, i2 = 0;
  for (size_t i = 0; i < num_samples; ++i) {
    adc_digi_output_data_t *p = &data[i];
    uint16_t chan = p->type1.channel;
    if (chan == channel[0])
      cap_buf[i1++] = p->type1.data;
    else if (chan == channel[1])
      cap_buf1[i2++] = p->type1.data;
  }
  if (rate < RATE_DUAL) {
    uint32_t *p = dma_buf, w;
    for (size_t i=0; i < i1/2; i++) {
      w = *p;
      *p++ = (w >> 16) | (w << 16);
    }

    p = dma_buf1;
    for (size_t i=0; i < i2/2; i++) {
      w = *p;
      *p++ = (w >> 16) | (w << 16);
    }
  }

  vTaskDelay(1);
  int t = trigger_point();
  if (ch0_mode != MODE_OFF)
    scaleDataArray(ad_ch0, t);
  if (ch1_mode != MODE_OFF)
    scaleDataArray(ad_ch1, t);
  vTaskDelay(1);
}
#else

void sample_dma(void) {
  adc_digi_output_data_t *data = (adc_digi_output_data_t *)dma_buf;
  uint32_t num_bytes_read = 0;

  adc_continuous_read(adc_handle, (uint8_t *)dma_buf, sizeof(dma_buf),
                      &num_bytes_read, 100);  // 100ms timeout
  int num_samples = num_bytes_read / sizeof(adc_digi_output_data_t);
  // Serial.println(num_samples);

  int i1 = 0, i2 = 0;
  for (size_t i = 0; i < num_samples; ++i) {
    adc_digi_output_data_t *p = &data[i];
    uint16_t chan = p->type2.channel;
    if (chan == channel[0])
      cap_buf[i1++] = p->type2.data;
    else if (chan == channel[1])
      cap_buf1[i2++] = p->type2.data;
  }

  vTaskDelay(1);
  int t = trigger_point();
  if (ch0_mode != MODE_OFF)
    scaleDataArray(ad_ch0, t);
  if (ch1_mode != MODE_OFF)
    scaleDataArray(ad_ch1, t);
  vTaskDelay(1);
}
#endif

#ifndef ESP32_C3
static const uint32_t sample_rate[8] = {
  250000, // 4us sampling (250ksps) x10
  250000, // 4us sampling (250ksps) x5
  250000, // 4us sampling (250ksps) x2
  250000, // 4us sampling (250ksps)      100u
  250000, // 8us sampling (125ksps) dual 200u
  100000, // 20us sampling (50ksps) dual 500u
   50000, // 40us sampling (25ksps) dual 1ms
   25000};// 80us sampling (12.5ksps) dual 2ms
#else
static const uint32_t sample_rate[8] = {
  83333,  // 12us sampling (83.3ksps) x10
  83333,  // 12us sampling (83.3ksps) x5
  83333,  // 12us sampling (83.3ksps) x2
  83333,  // 12us sampling (83.3ksps)
  50000,  // 20us sampling (50ksps)
  80000,  // 25us sampling (40ksps) dual
  40000,  // 50us sampling (20ksps) dual
  20000}; // 100us sampling (10ksps) dual
#endif

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num) {
  adc_continuous_handle_cfg_t adc_config = {
    .max_store_buf_size = NSAMP * sizeof(adc_digi_output_data_t),
    .conv_frame_size = NSAMP * sizeof(adc_digi_output_data_t),
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &adc_handle));

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = { 0 };
  for (int i = 0; i < channel_num; i++) {
    adc_pattern[i].atten = ADC_ATTEN_DB_11;
    adc_pattern[i].channel = channel[i] & 0x7;
    adc_pattern[i].unit = ADC_UNIT_1;
    adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
  }

  adc_continuous_config_t dig_cfg = {
    .pattern_num = channel_num,
    .adc_pattern = adc_pattern,
    .sample_freq_hz = sample_rate[rate],
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
#ifdef ESP32_C3
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
#else
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
#endif
  };
  ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));
  adc_continuous_start(adc_handle);
}

void rate_dma_mode_config(void) {
  adc_channel_t *ch = channel;
  if (ch0_mode == MODE_OFF && ch1_mode != MODE_OFF) {
    ch = &channel[1];
  } else {
    ch = channel;
  }
  if (rate <= RATE_DMA) {
    if (orate <= RATE_DMA)
      dma_adc_stop();
    if (rate >= RATE_DUAL)
      continuous_adc_init(channel, 2);  // initialize DMA ADC
    else
      continuous_adc_init(ch, 1);       // initialize DMA ADC
  } else if (orate <= RATE_DMA) {
    dma_adc_stop();
  }
}

void dma_adc_stop(void) {
  adc_continuous_stop(adc_handle);
  adc_continuous_deinit(adc_handle);  //stop & destroy continuous driver
}

int trigger_point(void) {
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
