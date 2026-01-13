/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * This examples creates a USB Microphone device using the TinyUSB
 * library and captures data from a PDM microphone using a sample
 * rate of 16 kHz, to be sent the to PC.
 * 
 * The USB microphone code is based on the TinyUSB audio_test example.
 * 
 * https://github.com/hathach/tinyusb/tree/master/examples/device/audio_test
 */

#include "pico/analog_microphone.h"
#include "usb_microphone.h"

static int16_t sample_buffer[SAMPLES_PER_FRAME];

const struct analog_microphone_config config =
{
  .gpio = 26,
  .bias_voltage = 1.25,
  .sample_rate = SAMPLE_RATE,
  .sample_buffer_size = SAMPLES_PER_FRAME
};

void on_samples_ready(void)
{
  analog_microphone_read(sample_buffer, SAMPLES_PER_FRAME);
  usb_microphone_write(sample_buffer);
}

int main(void)
{
  analog_microphone_init(&config);
  analog_microphone_set_samples_ready_handler(on_samples_ready);
  analog_microphone_start();

  usb_microphone_init();

  while (1)
  {
    usb_microphone_task();
  }
}
