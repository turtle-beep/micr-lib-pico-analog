/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Reinhard Panhuber
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

#include "usb_microphone.h"

/* ---------------- RANGE STRUCT (FIX) ---------------- */
typedef struct {
  uint16_t wNumSubRanges;
  struct {
    uint32_t bMin;
    uint32_t bMax;
    uint32_t bRes;
  } subrange[1];
} __attribute__((packed)) audio_freq_range_t;
/* --------------------------------------------------- */

// Audio controls
bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];

uint32_t sampFreq;
uint8_t clkValid;

// Volume range (OK as-is)
audio_control_range_2_n_t(1) volumeRng[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];

// Sample frequency range (FIXED)
static audio_freq_range_t sampleFreqRng;

static usb_microphone_tx_ready_handler_t usb_microphone_tx_ready_handler = NULL;

/*------------- MAIN -------------*/
void usb_microphone_init(void)
{
  tusb_init();

  sampFreq = SAMPLE_RATE;
  clkValid = 1;

  // Correct RANGE initialization (14 bytes total)
  sampleFreqRng.wNumSubRanges = 1;
  sampleFreqRng.subrange[0].bMin = SAMPLE_RATE;
  sampleFreqRng.subrange[0].bMax = SAMPLE_RATE;
  sampleFreqRng.subrange[0].bRes = 0;
}

void usb_microphone_set_tx_ready_handler(usb_microphone_tx_ready_handler_t handler)
{
  usb_microphone_tx_ready_handler = handler;
}

uint16_t usb_microphone_write(const void * data, uint16_t len)
{
  return tud_audio_write((uint8_t *)data, len);
}

void usb_microphone_task(void)
{
  tud_task();
}

/*------------------------------------------------------------------*/
/*-------------------- AUDIO CONTROL CALLBACKS ----------------------*/
/*------------------------------------------------------------------*/

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  uint8_t channelNum = TU_U16_LOW(p_request->wValue);
  uint8_t ctrlSel    = TU_U16_HIGH(p_request->wValue);
  uint8_t entityID   = TU_U16_HIGH(p_request->wIndex);

  /* -------- Feature Unit -------- */
  if (entityID == 2)
  {
    switch (ctrlSel)
    {
      case AUDIO_FU_CTRL_MUTE:
        return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);

      case AUDIO_FU_CTRL_VOLUME:
        if (p_request->bRequest == AUDIO_CS_REQ_CUR)
        {
          return tud_control_xfer(
            rhport, p_request,
            &volume[channelNum],
            sizeof(volume[channelNum])
          );
        }
        else if (p_request->bRequest == AUDIO_CS_REQ_RANGE)
        {
          audio_control_range_2_n_t(1) ret;
          ret.wNumSubRanges = 1;
          ret.subrange[0].bMin = -90;
          ret.subrange[0].bMax = 90;
          ret.subrange[0].bRes = 1;

          return tud_audio_buffer_and_schedule_control_xfer(
            rhport, p_request,
            &ret, sizeof(ret)
          );
        }
        break;
    }
  }

  /* -------- Clock Source -------- */
  if (entityID == 4)
  {
    switch (ctrlSel)
    {
      case AUDIO_CS_CTRL_SAM_FREQ:
        if (p_request->bRequest == AUDIO_CS_REQ_CUR)
        {
          // EXACTLY 4 bytes
          return tud_control_xfer(
            rhport, p_request,
            &sampFreq,
            sizeof(sampFreq)
          );
        }
        else if (p_request->bRequest == AUDIO_CS_REQ_RANGE)
        {
          // EXACTLY 14 bytes
          return tud_control_xfer(
            rhport, p_request,
            &sampleFreqRng,
            sizeof(sampleFreqRng)
          );
        }
        break;

      case AUDIO_CS_CTRL_CLK_VALID:
        return tud_control_xfer(
          rhport, p_request,
          &clkValid,
          sizeof(clkValid)
        );
    }
  }

  return false;
}

/*------------------------------------------------------------------*/
/*---------------------- STREAMING CALLBACKS ------------------------*/
/*------------------------------------------------------------------*/

bool tud_audio_tx_done_pre_load_cb(
  uint8_t rhport,
  uint8_t itf,
  uint8_t ep_in,
  uint8_t cur_alt_setting)
{
  (void) rhport;
  (void) itf;
  (void) ep_in;
  (void) cur_alt_setting;

  if (usb_microphone_tx_ready_handler)
    usb_microphone_tx_ready_handler();

  return true;
}

bool tud_audio_tx_done_post_load_cb(
  uint8_t rhport,
  uint16_t n_bytes_copied,
  uint8_t itf,
  uint8_t ep_in,
  uint8_t cur_alt_setting)
{
  (void) rhport;
  (void) n_bytes_copied;
  (void) itf;
  (void) ep_in;
  (void) cur_alt_setting;
  return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
{
  (void) rhport;
  (void) p_request;
  return true;
}
