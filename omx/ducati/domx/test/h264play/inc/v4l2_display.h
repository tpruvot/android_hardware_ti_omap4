/*
 * Copyright (c) 2010, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _V4L2_DISPLAY_H_
#define _V4L2_DISPLAY_H_

/* V4L2 related methods */

struct viddev
{
	char *path;
};

int v4l2_init(int num_buffers, int width, int height, int pixfmt,
    struct viddev *viddev);
void *v4l2_get_displayed_buffer(int timeout_ms);
void *v4l2_display_buffer(void *pBuffer);
int v4l2_display_done();
void *v4l2_get_start(int index);
size_t v4l2_get_length(int index);
int v4l2_get_stride();
int v4l2_set_crop(int x0, int y0, int width, int height);
#ifndef __NO_OMX__

#include <OMX_Core.h>
#include <OMX_IVCommon.h>

int omx_v4l2_init(int num_buffers, int width, int height,
    OMX_COLOR_FORMATTYPE eFormat, struct viddev *viddev);
int omx_v4l2_associate(int index, OMX_BUFFERHEADERTYPE * pBufHead);
OMX_BUFFERHEADERTYPE *omx_v4l2_display_buffer(OMX_BUFFERHEADERTYPE *
    pBufferOut);
OMX_BUFFERHEADERTYPE *omx_v4l2_get_displayed_buffer(int timeout_ms);
#define omx_v4l2_display_done() v4l2_display_done()
OMX_BUFFERHEADERTYPE *fomx_v4l2_display_buffer(OMX_BUFFERHEADERTYPE *
    pBufferOut, int w, int h, int s);

#endif

#endif
