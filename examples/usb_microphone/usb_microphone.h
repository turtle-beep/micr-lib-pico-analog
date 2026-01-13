/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _USB_MICROPHONE_H_
#define _USB_MICROPHONE_H_

#include "tusb.h"

#define SAMPLE_RATE        48000
#define SAMPLES_PER_FRAME  48

void usb_microphone_init(void);
void usb_microphone_task(void);
void usb_microphone_write(int16_t* samples);

#endif
