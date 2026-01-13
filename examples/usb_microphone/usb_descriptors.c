/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"

/* ================= Device ================= */

tusb_desc_device_t const desc_device =
{
  .bLength            = sizeof(tusb_desc_device_t),
  .bDescriptorType    = TUSB_DESC_DEVICE,
  .bcdUSB             = 0x0110,   // USB 1.1 → UAC1
  .bDeviceClass       = 0x00,
  .bDeviceSubClass    = 0x00,
  .bDeviceProtocol    = 0x00,
  .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
  .idVendor           = 0xCafe,
  .idProduct          = 0x4010,
  .bcdDevice          = 0x0100,
  .iManufacturer      = 0x01,
  .iProduct           = 0x02,
  .iSerialNumber      = 0x03,
  .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void)
{
  return (uint8_t const*)&desc_device;
}

/* ================= Configuration ================= */

#define ITF_AUDIO_CONTROL    0
#define ITF_AUDIO_STREAMING  1
#define ITF_TOTAL            2

#define EP_AUDIO_IN          0x81

#define CONFIG_TOTAL_LEN (9  /* config */ \
  + 9   /* AC interface */ \
  + 9   /* AC header */ \
  + 12  /* Input terminal */ \
  + 9   /* Output terminal */ \
  + 9   /* AS interface alt 0 */ \
  + 9   /* AS interface alt 1 */ \
  + 7   /* AS general */ \
  + 11  /* format type */ \
  + 9   /* endpoint */ \
  + 7)  /* endpoint CS */

uint8_t const desc_configuration[] =
{
  // Configuration
  9, TUSB_DESC_CONFIGURATION,
  1, ITF_TOTAL, 0,
  CONFIG_TOTAL_LEN & 0xff,
  CONFIG_TOTAL_LEN >> 8,
  0x00, 100,

  /* -------- Audio Control Interface -------- */
  9, TUSB_DESC_INTERFACE,
  ITF_AUDIO_CONTROL, 0, 0,
  TUSB_CLASS_AUDIO,
  AUDIO_SUBCLASS_CONTROL,
  0x00, 0,

  // AC Header
  9, AUDIO_CS_INTERFACE,
  AUDIO_CS_AC_HEADER,
  0x00, 0x01,   // UAC1
  9, 0,
  1, ITF_AUDIO_STREAMING,

  // Input Terminal (Microphone)
  12, AUDIO_CS_INTERFACE,
  AUDIO_CS_AC_INPUT_TERMINAL,
  0x01,
  0x01, 0x02,   // Microphone
  0x00,
  1,
  0x00, 0x00,
  0x00, 0x00,

  // Output Terminal (USB)
  9, AUDIO_CS_INTERFACE,
  AUDIO_CS_AC_OUTPUT_TERMINAL,
  0x02,
  0x01, 0x01,   // USB streaming
  0x00,
  0x01,
  0x00,

  /* -------- Audio Streaming Interface -------- */

  // AS alt 0 (zero bandwidth)
  9, TUSB_DESC_INTERFACE,
  ITF_AUDIO_STREAMING, 0, 0,
  TUSB_CLASS_AUDIO,
  AUDIO_SUBCLASS_STREAMING,
  0x00, 0,

  // AS alt 1 (operational)
  9, TUSB_DESC_INTERFACE,
  ITF_AUDIO_STREAMING, 1, 1,
  TUSB_CLASS_AUDIO,
  AUDIO_SUBCLASS_STREAMING,
  0x00, 0,

  // AS General
  7, AUDIO_CS_INTERFACE,
  AUDIO_CS_AS_GENERAL,
  0x01,
  0x01,
  0x01, 0x00,

  // Format Type I
  11, AUDIO_CS_INTERFACE,
  AUDIO_CS_AS_FORMAT_TYPE,
  AUDIO_FORMAT_TYPE_I,
  1,
  2,
  16,
  1,
  0x80, 0xBB, 0x00,  // 48000 Hz

  // ISO IN endpoint
  9, TUSB_DESC_ENDPOINT,
  EP_AUDIO_IN,
  0x05,
  CFG_TUD_AUDIO_EP_SZ_IN & 0xff,
  CFG_TUD_AUDIO_EP_SZ_IN >> 8,
  1,
  0x00,
  0x00,

  // Endpoint CS
  7, AUDIO_CS_ENDPOINT,
  AUDIO_CS_EP_GENERAL,
  0x00,
  0x00,
  0x00,
  0x00
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index;
  return desc_configuration;
}

/* ================= Strings ================= */

char const* string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 },
  "RP2040",
  "USB Microphone",
  "000001"
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t count;
  if (index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    count = 1;
  }
  else
  {
    const char* str = string_desc_arr[index];
    count = strlen(str);
    for (uint8_t i = 0; i < count; i++)
      _desc_str[1 + i] = str[i];
  }

  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * count + 2);
  return _desc_str;
}
