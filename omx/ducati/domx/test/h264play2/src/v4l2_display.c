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

#define __DEBUG__
#define __DEBUG_ASSERT__
#define __DEBUG_ENTRY__


#include <stdio.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <v4l2_display.h>
#include <utils.h>
#include <debug_utils.h>
#include <fcntl.h>
/*#include <linux/ioctl.h>*/
#include <linux/fb.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
/* #include <linux/errno.h> */
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/time.h>
struct timeval time0, time1;

#define VIDEO_DEVICE "/dev/video2"

static int v4l2_dev_display = -1;
static int v4l2_num_buffers = 0;
static int v4l2_bufs_streamed = 0;
static int v4l2_streaming = 0;
static int v4l2_bufs_in_queue = 0;
static int v4l2_type = 0;
static int v4l2_stride = 0;

#define __PSI_HACK__

static float v4l2_fps = 0.0;


#undef NV12_WORKAROUND
#ifdef NV12_WORKAROUND
#include <stdint.h>
#include <memmgr.h>
static int v4l2_width = 0;
static int v4l2_height = 0;
static int v4l2_nv12_stride = 0;


void convert_nv12_to_yuyv(unsigned char *nv12, unsigned short *yuyv, int w,
    int h, int s)
{
	P("converting %d*%d(s=%d) from %x to %x", w, h, s, nv12, yuyv);

	/* create yuyv data from nv12 */
	unsigned short *uv = (unsigned short *)(nv12 + h * s);
	int x, y;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x += 2)
		{
			*yuyv++ = (*nv12++) | ((*uv & 0xFF) << 8);
			*yuyv++ = (*nv12++) | (*uv++ & 0xFF00);
		}
		nv12 += s - w;
		uv += (s - w) >> 1;
		if (!(y & 1))
		{
			uv -= s >> 1;
		}
		yuyv += (v4l2_stride >> 1) - w;
	}
}

#endif
struct v4l2_display_buffer
{
	void *start;
	size_t length;
#ifdef  NV12_WORKAROUND
	void *workaround;
#endif
} *v4l2_display_buffers;

void *v4l2_get_start(int index)
{
	if (NOT_I(index, >=, 0) || NOT_I(index, <, v4l2_num_buffers))
		return NULL;
	return v4l2_display_buffers[index].start;
}

size_t v4l2_get_length(int index)
{
	if (NOT_I(index, >=, 0) || NOT_I(index, <, v4l2_num_buffers))
		return 0;
	return v4l2_display_buffers[index].length;
}

int v4l2_get_stride()
{
	return v4l2_stride;
}

int v4l2_set_crop(int x0, int y0, int width, int height)
{
	struct v4l2_crop crop;
	ZERO(crop);

	crop.type = v4l2_type;
	crop.c.left = x0;
	crop.c.top = y0;
	crop.c.width = width > 1920 ? 1920 : width;
	crop.c.height = height > 1080 ? 1080 : height;

	int res = ioctl(v4l2_dev_display, VIDIOC_S_CROP, &crop);
}

int v4l2_init(int num_buffers, int width, int height, int pixfmt,
    struct viddev *viddev)
{
	char *dev = viddev ? viddev->path : VIDEO_DEVICE;
	v4l2_dev_display = open(dev, O_RDWR);
	if (NOT_S(v4l2_dev_display, >=, 0))
		return (1);
	P("opened %s", dev);

	/* initialize display state */
	v4l2_bufs_streamed = v4l2_streaming = v4l2_bufs_in_queue = 0;

	struct v4l2_capability capability;
	struct v4l2_format format;
	struct v4l2_requestbuffers reqbuf;
	int i;

	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_QUERYCAP, &capability), ==,
		0))
		return (1);

	if (NOT_I(capability.capabilities & V4L2_CAP_STREAMING, !=, 0))
		return (1);

	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_G_FMT, &format), ==, 0))
		return (1);

	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.fmt.pix.bytesperline = 4096;
	format.fmt.pix.pixelformat = pixfmt;

#ifdef  NV12_WORKAROUND
	if (pixfmt == V4L2_PIX_FMT_NV12)
	{
		format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		P("overriding format: %c%c%c%c",
		    FOUR_CHARS(format.fmt.pix.pixelformat));
	}
#endif

	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_S_FMT, &format), ==, 0))
		return (1);

	/* get stride */
	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_G_FMT, &format), ==, 0))
		return (1);
	v4l2_stride = format.fmt.pix.bytesperline;
	P("stride=%d", v4l2_stride);

	/* :HACK: stride is 4K */
	v4l2_stride = 4096;

	reqbuf.type = v4l2_type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = num_buffers;

	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_REQBUFS, &reqbuf), ==, 0))
		return (1);

	P("Driver allocated %d buffers when %d are requested",
	    reqbuf.count, num_buffers);

	v4l2_num_buffers = reqbuf.count;
	ALLOCN(v4l2_display_buffers, v4l2_num_buffers);
	for (i = 0; i < v4l2_num_buffers; i++)
	{
		struct v4l2_buffer buffer;
		buffer.type = reqbuf.type;
		buffer.index = i;

		if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_QUERYBUF, &buffer),
			==, 0))
			return (1);

		P("%d: buffer.length=%d, buffer.m.offset=%d", i,
		    buffer.length, buffer.m.offset);
		v4l2_display_buffers[i].length = buffer.length;
		/*For MEM_MAP_V4L2 m.offset contains the offset for the start of the buffer
		   set by the device driver */
		v4l2_display_buffers[i].start =
		    mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
		    MAP_SHARED, v4l2_dev_display, buffer.m.offset);
		if (NOT_S(v4l2_display_buffers[i].start, !=, MAP_FAILED))
			return (1);

		P("%d: buffer.length=%d, buffer.start=%p", i,
		    v4l2_display_buffers[i].length,
		    v4l2_display_buffers[i].start);

#ifdef  NV12_WORKAROUND
		if (pixfmt == V4L2_PIX_FMT_NV12)
		{
			/* replace start with a tiler allocated buffer */
			v4l2_display_buffers[i].workaround =
			    v4l2_display_buffers[i].start;
			MemAllocBlock blocks[2];
			ZERO(blocks);
			blocks[0].pixelFormat = PIXEL_FMT_8BIT;
			blocks[0].dim.area.width = width;
			blocks[0].dim.area.height = height;
			blocks[1].pixelFormat = PIXEL_FMT_16BIT;
			blocks[1].dim.area.width = width >> 1;
			blocks[1].dim.area.height = height >> 1;
			v4l2_display_buffers[i].start =
			    MemMgr_Alloc(blocks, 2);
			if (NOT_S(v4l2_display_buffers[i].start, !=, 0))
				return (1);
			P("%d: workaround.start=%d", i,
			    v4l2_display_buffers[i].start);
			v4l2_width = width;
			v4l2_height = height;
			v4l2_nv12_stride = blocks[0].stride;
		} else
		{
			v4l2_display_buffers[i].workaround = NULL;
		}
#endif
	}

	return (0);
}

/**
 * Returns a displayed buffer from v4l2 within a timeout.
 *
 * Returns NULL if no buffers can be returned within the
 * timeout.  This call blocks.
 *
 * @author a0194118 (9/30/2009)
 *
 * @param timeout_ms  Timeout in ms.
 *
 * @return pointer to video buffer data.
 */
void *v4l2_get_displayed_buffer(int timeout_ms)
{
	struct v4l2_buffer filledbuffer;
	ZERO(filledbuffer);
	filledbuffer.type = v4l2_type;
	filledbuffer.memory = V4L2_MEMORY_MMAP;

	/* dequeue buffers if we have enough queued */
#ifdef __PSI_HASK__
	if (v4l2_bufs_in_queue > 1)
#else
	if (v4l2_bufs_in_queue > 3)
#endif
	{
		/* poll then deque */
		struct pollfd fd;
		fd.fd = v4l2_dev_display;
		fd.events = POLLIN;
		int result = poll(&fd, 1, timeout_ms);

		if (NOT_S(result, >, 0))
			return NULL;

		if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_DQBUF,
			    &filledbuffer), ==, 0))
			return NULL;

		P("DQ[%d] %d", filledbuffer.index, timeout_ms);
		v4l2_bufs_in_queue--;

		/* update pBufferOut for the actual buffer to be refilled */
		return v4l2_display_buffers[filledbuffer.index].start;
	}
	return NULL;
}

/**
 * Queues a buffer to v4l2, and returns a buffer that has been
 * processed.
 *
 * If buffer is not queued, it is returned.  If it is
 * successfully queued, another buffer is retrieved if possible.
 * If no buffer can be returned (as none is processed or due to
 * errors), NULL is returned.  This call blocks.
 *
 * @author a0194118 (9/30/2009)
 *
 * @param pBuffer   pointer to video buffer data
 *
 * @return pointer to a video buffer data that can be reused, or
 *         NULL if none can.
 */
void *v4l2_display_buffer(void *pBuffer)
{
	struct v4l2_buffer filledbuffer;
	int i;

	//Find out which of the allocated  buffers contains the data
	//later take into account nOffset and nFilledLen
	for (i = 0; pBuffer != v4l2_display_buffers[i].start; i++);
	if (NOT_I(i, <, v4l2_num_buffers))
		return pBuffer;

#ifdef NV12_WORKAROUND
	if (v4l2_display_buffers[i].workaround)
	{
		convert_nv12_to_yuyv(pBuffer,
		    v4l2_display_buffers[i].workaround, v4l2_width,
		    v4l2_height, v4l2_nv12_stride);
	}
#endif

#ifdef __NO_DSS2__
	if (!v4l2_streaming)
	{
		if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_STREAMON,
			    &v4l2_type), ==, 0))
			return pBuffer;
		v4l2_streaming = 1;
	}
#endif

	ZERO(filledbuffer);
	filledbuffer.type = v4l2_type;
	filledbuffer.memory = V4L2_MEMORY_MMAP;
	filledbuffer.index = i;

	/* :TRICKY: before the buffer is successfully queued, we need to return the
	   buffer because it would never otherwise be returned. */
	if (v4l2_bufs_streamed <= 10)
	{
		gettimeofday(&time0, NULL);
		P("Q[%d]", filledbuffer.index);
	} else
	{
		gettimeofday(&time1, NULL);
		v4l2_fps =
		    (time1.tv_sec - time0.tv_sec) +
		    0.000001 * (time1.tv_usec - time0.tv_usec);
		v4l2_fps /= (v4l2_bufs_streamed - 10);
		if (v4l2_fps)
			v4l2_fps = 1 / v4l2_fps;
		P("Q[%d] fps = %4.2g T=%d ms", filledbuffer.index, v4l2_fps,
		    ((time1.tv_sec & 0xF) * 1000 + time1.tv_usec / 1000));
	}

	if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_QBUF, &filledbuffer), ==, 0))
		return pBuffer;

	v4l2_bufs_in_queue++;
	v4l2_bufs_streamed++;

	/* start streaming */
#ifdef __PSI_HACK__
	if (v4l2_bufs_streamed >= 4 && !v4l2_streaming)
#else
	if (v4l2_bufs_streamed >= 2 && !v4l2_streaming)
#endif
	{
		if (NOT_S(ioctl(v4l2_dev_display, VIDIOC_STREAMON,
			    &v4l2_type), ==, 0))
			return NULL;
		v4l2_streaming = 1;
#ifdef __PSI_HACK__
		usleep(4 * 330000);
		gettimeofday(&time0, NULL);
#endif
	}
#ifdef __PSI_HACK__
	return v4l2_get_displayed_buffer(1);
#else
	return v4l2_get_displayed_buffer(1000);
#endif
}

int v4l2_display_done()
{
	int res = 0, i;
	struct v4l2_buffer filledbuffer;
	ZERO(filledbuffer);
	filledbuffer.type = v4l2_type;
	filledbuffer.memory = V4L2_MEMORY_MMAP;

	PS("fps = %4.2f", v4l2_fps);

	/* dequeue remaining buffers */
	while (v4l2_bufs_in_queue > 1)
	{
		// ioctl(v4l2d, VIDIOC_DQBUF, ix);
		ERR_ADD_S(res, ioctl(v4l2_dev_display, VIDIOC_DQBUF,
			&filledbuffer));
		// P("DQ[%d]", filledbuffer.index);
		v4l2_bufs_in_queue--;
	}

	/* unmap the buffers */
	for (i = 0; i < v4l2_num_buffers; i++)
	{
#ifdef NV12_WORKAROUND
		if (v4l2_display_buffers[i].workaround)
		{
			MemMgr_Free(v4l2_display_buffers[i].start);
			v4l2_display_buffers[i].start =
			    v4l2_display_buffers[i].workaround;
		}
#endif
		if (v4l2_display_buffers[i].start)
			munmap(v4l2_display_buffers[i].start,
			    v4l2_display_buffers[i].length);
	}

	if (v4l2_streaming)
	{
		ERR_ADD_S(res, ioctl(v4l2_dev_display, VIDIOC_STREAMOFF,
			&v4l2_type));
	}

	/* close display */
	close(v4l2_dev_display);
	return (res);
}

#ifndef __NO_OMX__
#include <OMX_Component.h>
struct omx_display_buffer
{
	OMX_BUFFERHEADERTYPE *pBufHead;
} *omx_display_buffers;

int omx_v4l2_init(int num_buffers, int width, int height,
    OMX_COLOR_FORMATTYPE eFormat, struct viddev *viddev)
{
	int pixelformat = (eFormat == OMX_COLOR_Format16bitBGR565 ? :
	    eFormat == OMX_COLOR_Format24bitRGB888 ? :
	    eFormat == OMX_COLOR_Format32bitARGB8888 ? :
	    eFormat == OMX_COLOR_FormatYCbYCr ? :
	    eFormat == OMX_COLOR_FormatCbYCrY ? :
	    eFormat == OMX_COLOR_FormatYUV420PackedSemiPlanar ? :
	    V4L2_PIX_FMT_RGB565);

	P("OMX Format: %x pixel Format: %c%c%c%c\n", eFormat,
	    FOUR_CHARS(pixelformat));
	pixelformat = V4L2_PIX_FMT_NV12;
	P("now OMX Format: %x pixel Format: %c%c%c%c\n", eFormat,
	    FOUR_CHARS(pixelformat));

	int res = v4l2_init(num_buffers, width, height, pixelformat, viddev);
	ALLOCN(omx_display_buffers, num_buffers);
	if (res == 0)
	{
		int i;
		for (i = 0; i < v4l2_num_buffers; i++)
			omx_display_buffers[i].pBufHead = NULL;
	}
	return res;
}

int omx_v4l2_associate(int index, OMX_BUFFERHEADERTYPE * pBufHead)
{
	omx_display_buffers[index].pBufHead = pBufHead;
}

OMX_BUFFERHEADERTYPE *omx_v4l2_display_buffer(OMX_BUFFERHEADERTYPE *
    pBufferOut)
{
	/* get actual video buffer address */
	void *pBuffer = pBufferOut->pBuffer;
	// P("nOffset = %08x", pBufferOut->nOffset);

	/* display buffer */
	pBuffer = v4l2_display_buffer(pBuffer);
	if (!pBuffer)
		return NULL;

	/* check if this is the same buffer */
	if (NOT_P(pBuffer, !=, pBufferOut->pBuffer))
		return pBufferOut;

	/* retrieve OMX buffer header from buffer address */
	int i;
	for (i = 0;
	    i < v4l2_num_buffers && v4l2_display_buffers[i].start != pBuffer;
	    i++);
	if (NOT_I(i, <, v4l2_num_buffers))
		return NULL;
	return omx_display_buffers[i].pBufHead;
}

OMX_BUFFERHEADERTYPE *omx_v4l2_get_displayed_buffer(int timeout_ms)
{
	/* get video buffer from v4l2 */
	void *pBuffer = v4l2_get_displayed_buffer(timeout_ms);

	/* handle NULL */
	if (!pBuffer)
		return NULL;

	/* retrieve OMX buffer header from buffer address */
	int i;
	for (i = 0;
	    i < v4l2_num_buffers && v4l2_display_buffers[i].start != pBuffer;
	    i++);
	if (NOT_I(i, <, v4l2_num_buffers))
		return NULL;

	return omx_display_buffers[i].pBufHead;
}

#endif
