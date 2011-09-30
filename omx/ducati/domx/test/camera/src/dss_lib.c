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

/**
* @file dss_lib.c
*
* This File contains functions for call DSS ioctls.
*
*
* @path  domx/test/camera/inc
*
* @rev  0.1
*/
/* ----------------------------------------------------------------------------
*!
*! Revision History
*! =======================================================================
*!  Initial Version
* ========================================================================*/
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <sys/types.h>
#include "dss_lib.h"
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define VIDEO_DEVICE1 "/dev/video1"

#define debug	0
#define dprintf(level, fmt, arg...) do {			\
		if (debug >= level) {					\
			printf("DAEMON: " fmt , ## arg); } } while (0)

/*========================================================*/
/* @ fn getDSSBuffers : Get Buffers allocated through DSS */
/*========================================================*/
uint getDSSBuffers(uint count, struct dss_buffers *buffers, int vid1_fd)
{
	int result;
	struct v4l2_requestbuffers reqbuf;
	int i;

	reqbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = count;
	result = ioctl(vid1_fd, VIDIOC_REQBUFS, &reqbuf);
	if (result != 0)
	{
		perror("VIDEO_REQBUFS");
		return -1;
	}
	dprintf(2, "\nDSS driver allocated %d buffers \n", reqbuf.count);
	if (reqbuf.count != count)
	{
		dprintf(0, "\nDSS DRIVER FAILED To allocate the requested ");
		dprintf(0,
		    " Number of Buffers This is a stritct error check ");
		dprintf(0, " So the application may fail.\n");
	}

	for (i = 0; i < reqbuf.count; ++i)
	{
		struct v4l2_buffer buffer;
		buffer.type = reqbuf.type;
		buffer.index = i;

		result = ioctl(vid1_fd, VIDIOC_QUERYBUF, &buffer);
		if (result != 0)
		{
			perror("VIDIOC_QUERYBUF");
			return -1;
		}
		dprintf(2, "%d: buffer.length=%d, buffer.m.offset=%d\n",
		    i, buffer.length, buffer.m.offset);
		buffers[i].length = buffer.length;
		buffers[i].start = mmap(NULL, buffer.length, PROT_READ |
		    PROT_WRITE, MAP_SHARED, vid1_fd, buffer.m.offset);
		if (buffers[i].start == MAP_FAILED)
		{
			perror("mmap");
			return -1;
		}

		dprintf(0, "Buffers[%d].start = %x  length = %d\n", i,
		    buffers[i].start, buffers[i].length);
	}
	return 0;

}

/*========================================================*/
/* @ fn getv4l2pixformat: Get V4L2 Pixel format based on  */
/* the format input from user                             */
/*========================================================*/
unsigned int getv4l2pixformat(const char *image_fmt)
{
	if (!strcmp(image_fmt, "YUYV"))
		return V4L2_PIX_FMT_YUYV;
	else if (!strcmp(image_fmt, "UYVY"))
		return V4L2_PIX_FMT_UYVY;
	else if (!strcmp(image_fmt, "RGB565"))
		return V4L2_PIX_FMT_RGB565;
	else if (!strcmp(image_fmt, "RGB565X"))
		return V4L2_PIX_FMT_RGB565X;
	else if (!strcmp(image_fmt, "RGB24"))
		return V4L2_PIX_FMT_RGB24;
	else if (!strcmp(image_fmt, "RGB32"))
		return V4L2_PIX_FMT_RGB32;
	else if (!strcmp(image_fmt, "NV12"))
		return V4L2_PIX_FMT_NV12;
	else
	{
		printf("fmt has to be NV12, UYVY, RGB565, RGB32, "
		    "RGB24, UYVY or RGB565X!\n");
		return -1;
	}
}

/*========================================================*/
/* @ fn SetFormatforDSSvid : Set the resolution width and */
/* height of DSS Driver                                   */
/*========================================================*/
int SetFormatforDSSvid(unsigned int width, unsigned int height,
    const char *image_fmt, int vid1_fd)
{
	struct v4l2_format format;
	int result;
	unsigned int v4l2pixformat;
	v4l2pixformat = getv4l2pixformat(image_fmt);
	if (v4l2pixformat == -1)
	{
		printf("Pixel Format is not supported\n");
		return -1;
	}

	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	format.fmt.pix.pixelformat = v4l2pixformat;
	/* set format of the picture captured */
	result = ioctl(vid1_fd, VIDIOC_S_FMT, &format);
	if (result != 0)
	{
		perror("VIDIOC_S_FMT");
		return result;
	}
	return 0;
}

/*========================================================*/
/* @ fn open_video1 : Open Video1 Device                  */
/*========================================================*/
int open_video1()
{
	int result;
	struct v4l2_capability capability;
	struct v4l2_format format;
	int vid1_fd;
	vid1_fd = open(VIDEO_DEVICE1, O_RDWR);
	if (vid1_fd <= 0)
	{
		dprintf(0, "\n Failed to open the DSS Video Device\n");
		return -1;
	} else
		dprintf(3, "\nVideo 1 opened successfully \n");
	dprintf(2, "\nopened Video Device 1 for preview \n");
	result = ioctl(vid1_fd, VIDIOC_QUERYCAP, &capability);
	if (result != 0)
	{
		perror("VIDIOC_QUERYCAP");
		return -1;
	}
	if (capability.capabilities & V4L2_CAP_STREAMING == 0)
	{
		dprintf(0, "VIDIOC_QUERYCAP indicated that driver is not "
		    "capable of Streaming \n");
		return -1;
	}
	format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	result = ioctl(vid1_fd, VIDIOC_G_FMT, &format);

	if (result != 0)
	{
		perror("VIDIOC_G_FMT");
		return -1;
	}
	return vid1_fd;
}

/*========================================================*/
/* @ fn SendbufferToDss :Sending buffers to DSS for       */
/* displaying                                             */
/*========================================================*/
int SendbufferToDss(int index, int vid1_fd)
{
	struct v4l2_buffer filledbuffer;
	int result;
	static int time_to_streamon;

	dprintf(2, "\n In SendbufferToDss() \n");
	filledbuffer.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	filledbuffer.memory = V4L2_MEMORY_MMAP;
	filledbuffer.flags = 0;
	filledbuffer.index = index;

	dprintf(2, "\n Before DSS VIDIOC_QBUF()of buffer index %d\n", index);
	result = ioctl(vid1_fd, VIDIOC_QBUF, &filledbuffer);
	if (result != 0)
	{
		perror("VIDIOC_QBUF FAILED ");
		dprintf(0, "filldbuffer.index = %d \n", filledbuffer.index);
	}

	++time_to_streamon;
	if (time_to_streamon == 1)
	{
		dprintf(3, "\n STREAM ON DONE !!! \n");
		result = ioctl(vid1_fd, VIDIOC_STREAMON, &filledbuffer.type);
		dprintf(3, "\n STREAMON result = %d\n", result);
		if (result != 0)
			perror("VIDIOC_STREAMON FAILED");
	}
	filledbuffer.flags = 0;
	if (time_to_streamon > 1)
	{
		dprintf(3, "\n DQBUF CALLED \n");
		result = ioctl(vid1_fd, VIDIOC_DQBUF, &filledbuffer);
		dprintf(3, "\n DQBUF buffer index got = %d\n",
		    filledbuffer.index);
		if (result != 0)
			perror("VIDIOC_DQBUF FAILED ");
	} else
		filledbuffer.index = 0xFF;
	dprintf(3, "Returning from SendbufferToDss\n");
	return filledbuffer.index;
}
