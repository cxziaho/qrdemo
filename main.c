/*
 * QR Reader Demo by cxziaho
 * Uses library https://github.com/dlbeer/quirc (ported to Vita by me)
 * Probably a bad example, more of a PoC (terrible threading, poor memory management, etc)
 * 
 * Camera init and de-init code from LuaPlayer Plus
 *
 * MIT License
 * 
 * Copyright (c) 2017 cxz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <psp2/display.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h> 
#include <psp2/kernel/sysmem.h>
#include <psp2/appmgr.h> 
#include <psp2/ctrl.h>
#include <psp2/camera.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <quirc.h>
#include <vita2d.h>

#define CAM_WIDTH 640
#define CAM_HEIGHT 360

#define MAX_STRING 128

struct quirc *qr;
uint32_t* qr_data;
int qr_next;

int cur_cam = 1;
vita2d_texture *camera_tex;
vita2d_pgf *pgf;

SceCameraInfo cam_info;
SceCameraRead cam_info_read;

char last_text[MAX_STRING];
char last_qr[MAX_STRING];
int last_qr_len;

int qrThread() {
	qr = quirc_new();
	if (!qr) {
		snprintf(last_text, MAX_STRING, "QR Init Failed");
	}
	if (quirc_resize(qr, CAM_WIDTH/2, CAM_HEIGHT/2) < 0) {
		snprintf(last_text, MAX_STRING, "QR Resize Failed");
	}
	
	qr_next = 1;
	while (1) {
		if (qr_next == 0) {
			uint8_t *image;
			int w, h;
			image = quirc_begin(qr, &w, &h);
					
			uint8_t red, green, blue;
			uint32_t colourRGBA;
			int y;
			for (y = 0; y < h; y++) {
				int x;
				for (x = 0; x < w; x++) {
					colourRGBA = qr_data[((y*2)*CAM_WIDTH)+x*2];
					red = (colourRGBA & 0x000000FF);
					green = (colourRGBA & 0x0000FF00) >> 8;
					blue = (colourRGBA & 0x00FF0000) >> 16;
					image[(y*(CAM_WIDTH/2))+x] = (red + green + blue) / 3;
				}
			}
			quirc_end(qr);
			int i = 0;
			int num_codes = quirc_count(qr);
			if (num_codes > 0) {
				struct quirc_code code;
				struct quirc_data data;
				quirc_decode_error_t err;
				
				quirc_extract(qr, 0, &code);
				err = quirc_decode(&code, &data);
				if (err) {
				} else {
					snprintf(last_text, MAX_STRING, "Decoded: %s", data.payload);
					last_qr_len = snprintf(last_qr, MAX_STRING, data.payload);
				}
			} else {
				snprintf(last_text, MAX_STRING, "Scanning");
				memset(last_qr, 0, MAX_STRING);
				last_qr_len = 0;
			}
			qr_next = 1;
		}
	}
}

int is_website() {
	if (last_qr_len > 4)
		if (last_qr[0] == 'h' && last_qr[1] == 't' && last_qr[2] == 't' && last_qr[3] == 'p')
			return 1;
	return 0;
}

int initCamera() {
	SceKernelMemBlockType orig = vita2d_texture_get_alloc_memblock_type();
	vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
	camera_tex = vita2d_create_empty_texture(CAM_WIDTH, CAM_HEIGHT);
	vita2d_texture_set_alloc_memblock_type(orig);
	
	cam_info.size = sizeof(SceCameraInfo);
	cam_info.format = SCE_CAMERA_FORMAT_ABGR;
	cam_info.resolution = SCE_CAMERA_RESOLUTION_640_360;
	cam_info.pitch = vita2d_texture_get_stride(camera_tex) - (CAM_WIDTH << 2);
	cam_info.sizeIBase = (CAM_WIDTH * CAM_HEIGHT) << 2;
	cam_info.pIBase = vita2d_texture_get_datap(camera_tex);
	cam_info.framerate = 30;
	
	cam_info_read.size = sizeof(SceCameraRead);
	cam_info_read.mode = 0;
	sceCameraOpen(cur_cam, &cam_info);
	sceCameraStart(cur_cam);
	return 0;
}

int renderCamera() {
	sceCameraRead(cur_cam, &cam_info_read);
	vita2d_draw_texture(camera_tex, (960/2)-(CAM_WIDTH/2), (540/2)-(CAM_HEIGHT/2));
	if (qr_next) {
		qr_data = (uint32_t *)vita2d_texture_get_datap(camera_tex);
		qr_next = 0;
	}
	return 0;
}

int exitCamera() {
	sceCameraStop(cur_cam);
	sceCameraClose(cur_cam);
	vita2d_free_texture(camera_tex);
	return 0;
}

int main(int argc, char *argv[]) {
	SceUID thid = sceKernelCreateThread("qr_decode_thread", qrThread, 0x40, 0x100000, 0, 0, NULL);
	if (thid >= 0) sceKernelStartThread(thid, 0, NULL);
	
	vita2d_init();
	vita2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0x00));
	initCamera();
	
	snprintf(last_text, MAX_STRING, "Scanning");
	
	pgf = vita2d_load_default_pgf();
	
	SceCtrlData ctrl_peek;SceCtrlData ctrl_press;
	memset(&ctrl_peek, 0, sizeof(ctrl_peek));
	memset(&ctrl_press, 0, sizeof(ctrl_press));
	
	while (1) {
		ctrl_press = ctrl_peek;
		sceCtrlPeekBufferPositive(0, &ctrl_peek, 1);
		ctrl_press.buttons = ctrl_peek.buttons & ~ctrl_press.buttons;
		if (ctrl_press.buttons & SCE_CTRL_START) {
			break;
		} else if (ctrl_press.buttons & SCE_CTRL_CIRCLE && is_website()) {
			sceAppMgrLaunchAppByUri(0x20000, last_qr);
		}
		
		vita2d_start_drawing();
		vita2d_clear_screen();
		
		vita2d_pgf_draw_text(pgf, 10, 20, RGBA8(0,255,0,255), 1.0f, "QR Demo by cxziaho");
		vita2d_pgf_draw_text(pgf, 10, 40, RGBA8(255,255,255,255), 1.0f, "Scan a QR Code and see its text.");
		
		if (is_website()) {
			vita2d_pgf_draw_text(pgf, 10, 520, RGBA8(255,255,255,255), 1.0f, "Press \u25CB to open this website.");
		}
		
		renderCamera();
		vita2d_pgf_draw_text(pgf, 10, 540, RGBA8(255,255,255,255), 1.0f, last_text);
	
		vita2d_end_drawing();
		vita2d_swap_buffers();
	}
	
	vita2d_fini();
	exitCamera();
	vita2d_free_pgf(pgf);
	sceKernelDeleteThread(thid);
	quirc_destroy(qr);
	sceKernelExitProcess(0);
	return 0;
}
