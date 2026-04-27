#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "zap-vga16-raw-neg.h"
#include "keyboard_input.h"

#define WIDTH 320
#define HEIGHT 240

#define PSF_GLYPH_SIZE 16

uint8_t cursor[PSF_GLYPH_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

struct typewrt {
  unsigned char *buf;
  int modkeys;       
  int curx;
  int cury;
  int curmode;
  uint8_t curbuf[PSF_GLYPH_SIZE];
  Display *d;
  Window w;
  GC gc;
  XImage *img;
};

void typewrt_init(struct typewrt *t) {
  t->d = XOpenDisplay(NULL);
  int s = DefaultScreen(t->d);
  t->w = XCreateSimpleWindow(t->d, RootWindow(t->d, s), 10, 10, WIDTH, HEIGHT, 1,
                                   BlackPixel(t->d, s), WhitePixel(t->d, s));
  t->gc = DefaultGC(t->d, s);
  XSelectInput(t->d, t->w, ExposureMask | KeyPressMask | KeyReleaseMask );
  //XStoreName(t->d, t->w, t->title);
  XMapWindow(t->d, t->w);
  //XSync(t->d, t->w);
  t->buf = malloc(WIDTH * HEIGHT / 8);
  memset(t->buf, 0xff, WIDTH * HEIGHT / 8); // Pattern
  
  t->img = XCreateImage(t->d, DefaultVisual(t->d, s), 1, XYBitmap, 0,
                        (char *)t->buf, WIDTH, HEIGHT, 8, 0);

  // 1-bit depth, 8 bits per byte = 1 byte per 8 pixels
  // Start cursor at 0,0
  t->curx = 0;
  t->cury = 0;
  t->curmode = 0;
}

void typewrt_close(struct typewrt *t) { XCloseDisplay(t->d); }


void displayChar(uint8_t index, struct typewrt *t) {
    for (int m =0; m < PSF_GLYPH_SIZE; m++) {
       t->buf[(( (t->cury) * PSF_GLYPH_SIZE +m)*WIDTH + 8 * (t->curx)) / 8] = zap_vga16_psf[ index * PSF_GLYPH_SIZE +m];
       t->curbuf[m] = t->buf[(( (t->cury) * PSF_GLYPH_SIZE +m)*WIDTH + 8 * (t->curx)) / 8];
    }
}

void setCur(struct typewrt *t, int curx, int cury) {
    for (int m =0; m < PSF_GLYPH_SIZE; m++) {
       t->buf[(( (t->cury) * PSF_GLYPH_SIZE +m)*WIDTH + 8 * (t->curx)) / 8] = t->curbuf[m];
       t->buf[(( cury * PSF_GLYPH_SIZE +m)*WIDTH + 8 * curx) / 8] = cursor[m];
    } 
    t->curx = curx;
    t->cury = cury;
}

int main() {

    struct typewrt t = {};
    typewrt_init(&t);

    XEvent e;
    while (1) {
        XNextEvent(t.d, &e);
        if (e.type == Expose) {
            setCur(&t, t.curx, t.cury);
            XPutImage(t.d, t.w, t.gc, t.img, 0, 0, 0, 0, WIDTH, HEIGHT);
        }
        if (e.type == KeyPress) {
            // Update image data on key press (invert)
            Virtual_Key vk = keymap[ (keyeventLUT[e.xkey.keycode] & KEY_MASK) ];
	    if (vk >= VKCHAROFFSET) {
	        uint8_t fontchar = fontmap[vk - VKCHAROFFSET];
                displayChar(fontchar, &t);
                setCur(&t, t.curx +1, t.cury);
	    }
	    else if (vk == VK_ENTER) {
                setCur(&t, 0, t.cury +1);
	    };
            XPutImage(t.d, t.w, t.gc, t.img, 0, 0, 0, 0, WIDTH, HEIGHT);
	    XFlush(t.d);
	    if (e.xkey.keycode == 9) break; 
        }
    }
    typewrt_close(&t);
    return 0;
}


