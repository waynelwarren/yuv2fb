/* Copyright (c) 2018, Curtis McEnroe <programble@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

uint8_t clamp(uint32_t x, uint32_t min, uint32_t max)
{
    if (x < min) {
        x = min;
    }
    else if (x > max) {
        x = max;
    }
    return x;
}

int main(int argc, char *argv[]) {
    size_t len;
    char *cam;
    uint32_t scr_top = 0;
    uint32_t scr_left = 0;
    uint32_t img_width;
    uint32_t img_height;

    if (argc >= 2) {
        cam = argv[1];
    } else {
        fprintf(stderr, "Usage: img YUV-frame width height [left [top]]\n");
        exit(1);
    }
    if (argc >= 3)
        img_width = atoi(argv[2]);
    else
        img_width = 0;

    if (argc >= 4) 
        img_height = atoi(argv[3]);
    else
        img_height = 0;

    if (img_height == 0 || img_width == 0) {
        fprintf(stderr, "Usage: img YUV-frame left top\n");
        exit(1);
    }

    if (argc >= 5) 
        scr_left = atoi(argv[4]);
    else
        scr_left = 0;

    if (argc >= 6) 
        scr_top = atoi(argv[5]);
    else
        scr_top = 0;

    const char *fbPath = "/dev/fb0";

    int fb = open(fbPath, O_RDWR);
    if (fb < 0) err(EX_OSFILE, "%s", fbPath);

    struct fb_var_screeninfo info;
    int error = ioctl(fb, FBIOGET_VSCREENINFO, &info);
    if (error) err(EX_IOERR, "%s", fbPath);
    printf("info.xres=%d, yres=%d\n", info.xres, info.yres);

    size_t size = 4 * info.xres * info.yres;
    uint8_t *buf = (uint8_t *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (buf == MAP_FAILED) err(EX_IOERR, "%s", fbPath);

    int seq = 0;
    FILE *fp = fopen(cam, "r");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open file '%s'\n", cam);
        exit(1);
    }

    size_t size_total = img_width*img_height;
    size_t img_size = img_width*img_height+(img_width*img_height)/2;
    uint8_t img[img_size];
    for (;;) {
        printf("Reading new frame, %ld bytes\n", img_size);
        if (fread(img, 1, img_size, fp) <= 0) {
            fprintf(stderr, "Unable to read %ld bytes from '%s'\n", img_size, cam);
            exit(1);
        }

        uint8_t y_val, u_val, v_val;
        uint8_t yValue, uValue, vValue;
        uint32_t rTmp, gTmp, bTmp;
        uint32_t buf_offset;
        for (uint32_t y=0; y<img_height; y++) {
            buf_offset = (scr_top+y)*info.xres*4 + scr_left*4;
            for (uint32_t x=0; x<img_width; x++) {
                y_val = img[y*img_width + x];
                v_val = img[(y/2)*(img_width/2) + x/2 + size_total];
                u_val = img[(y/2)*(img_width/2) + x/2 + size_total + size_total/4];
                rTmp = y_val + (1.370705 * (v_val-128));
                gTmp = y_val - (0.698001 * (v_val-128)) - (0.337633 * (u_val-128));
                bTmp = y_val + (1.732446 * (u_val-128));
                buf[buf_offset++] = clamp(rTmp, 0, 255);
                buf[buf_offset++] = clamp(gTmp, 0, 255);
                buf[buf_offset++] = clamp(bTmp, 0, 255);
                buf[buf_offset++] = 0xff;
            }
        }
        exit(0);

        // slow motion
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 3333 * 10000L;
        nanosleep(&req, (struct timespec *)NULL);
    }
    fclose(fp);
}
