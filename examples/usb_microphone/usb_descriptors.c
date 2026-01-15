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

#define AUDIO_CS_AS_EP_ATTR_NO_PITCH_CTRL 0x00
#define AUDIO_CS_EP_SUBTYPE_GENERAL       0x01

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]     AUDIO | MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
    _PID_MAP(MIDI, 3) | _PID_MAP(AUDIO, 4) | _PID_MAP(VENDOR, 5) )

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum
{
    ITF_NUM_AUDIO_CONTROL = 0,
    ITF_NUM_AUDIO_STREAMING,
    ITF_NUM_TOTAL
};

#if CFG_TUSB_MCU == OPT_MCU_LPC175X_6X || CFG_TUSB_MCU == OPT_MCU_LPC177X_8X || CFG_TUSB_MCU == OPT_MCU_LPC40XX
#define EPNUM_AUDIO   0x03
#else
#define EPNUM_AUDIO   0x01
#endif

//#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + 109) // Update total length manually if needed
#define CONFIG_TOTAL_LEN sizeof(desc_configuration)

uint8_t const desc_configuration[] =
{
    // Configuration descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // ---- Audio Control Interface ----
    TUD_AUDIO_DESC_IAD(ITF_NUM_AUDIO_CONTROL, 2, 0x00),

    // Standard AC Interface
    TUD_AUDIO_DESC_STD_AC(ITF_NUM_AUDIO_CONTROL, 0, 0x00),

    // Clock Source descriptor (FIX: bControlSize = 4)
    0x0A,           // bLength
    0x24,           // bDescriptorType = CS_INTERFACE
    0x0A,           // bDescriptorSubType = CLOCK_SOURCE
    0x04,           // bClockID
    0x01,           // bmAttributes (Internal Fixed Clock)
    0x01,           // bmControls (Only CUR valid)
    4,              // bControlSize = 4 (matches firmware)
    0x00,           // bmFormats
    0x00,           // bEndpoint
    0x00,           // Reserved

    // Input Terminal (Microphone)
    0x0C,           // bLength
    0x24,           // bDescriptorType
    0x02,           // bDescriptorSubType = INPUT_TERMINAL
    0x01,           // bTerminalID
    0x01, 0x02,     // wTerminalType (Generic Microphone)
    0x00,           // bAssocTerminal
    0x04,           // bCSourceID (Clock Source)
    0x01,           // bNrChannels
    0x00, 0x00, 0x00,// bmChannelConfig
    0x00,           // iChannelNames

    // Feature Unit
    0x07,           // bLength
    0x24,           // bDescriptorType
    0x06,           // bDescriptorSubType = FEATURE_UNIT
    0x02,           // bUnitID
    0x01,           // bSourceID (Input Terminal)
    0x03, 0x00,     // bmaControls(0) Master Channel (Mute + Volume)
    0x00,           // iFeature

    // Output Terminal
    0x09,           // bLength
    0x24,           // bDescriptorType
    0x03,           // bDescriptorSubType = OUTPUT_TERMINAL
    0x03,           // bTerminalID
    0x01, 0x01,     // wTerminalType (USB Streaming)
    0x00,           // bAssocTerminal
    0x02,           // bSourceID (Feature Unit)
    0x00,           // iTerminal

    // ---- Audio Streaming Interface ----
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 0, 0,0),
    TUD_AUDIO_DESC_STD_AS_INT(ITF_NUM_AUDIO_STREAMING, 1, 0,0),

    // AS General
    0x07,           // bLength
    0x24,           // bDescriptorType
    0x01,           // bDescriptorSubType = AS_GENERAL
    0x03,           // bTerminalLink (Output Terminal ID)
    0x04,           // bCSourceID (Clock Source)
    0x01,           // bmControls
    0x00,           // bFormatType

    // Format Type
    0x0B,           // bLength
    0x24,           // bDescriptorType
    0x02,           // bDescriptorSubType = FORMAT_TYPE
    0x01,           // bFormatType = FORMAT_TYPE_I
    0x01,           // bNrChannels
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX*8, // bSubFrameSize
    CFG_TUD_AUDIO_FUNC_1_N_BYTES_PER_SAMPLE_TX,   // bBitResolution
    0x01,           // bSamFreqType
    //0x80, 0x3E, 0x00, // tSamFreq[0] = 16kHz example (LSB first)
    0x80, 0xBB, 0x00  // 48,000 Hz (LSB first)

    // Iso IN Endpoint
    TUD_AUDIO_DESC_STD_AS_ISO_EP(
        0x80 | EPNUM_AUDIO,
        TUSB_XFER_ISOCHRONOUS,
        CFG_TUD_AUDIO_EP_SZ_IN,
        1
    ),

    // Iso Endpoint CS
    TUD_AUDIO_DESC_CS_AS_ISO_EP(
        AUDIO_CS_AS_EP_ATTR_NO_PITCH_CTRL,
        0,
        0,
        0
    )
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
char const* string_desc_arr [] =
{
    (const char[]) { 0x09, 0x04 }, 	// 0: English
    "PaniRCorp",                   	// 1: Manufacturer
    "MicNode",              		// 2: Product
    "123456",                      	// 3: Serial
    "UAC2",                 	 	// 4: Audio Interface
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else
    {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        for(uint8_t i=0;i<chr_count;i++) _desc_str[1+i] = str[i];
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2*chr_count + 2);
    return _desc_str;
}
