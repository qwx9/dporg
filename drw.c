#include <u.h>
#include <libc.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

static u32int fb[Vwidth * Vheight];

int npal;
u32int *pal;
Pic pics[PCend];

static int scale, fbsz;
static Rectangle fbsr;
static Image *fbs;
static u32int *fbsbuf;

static void
drawscaled(void)
{
	u32int *s, *p, v;

	s = fb;
	p = fbsbuf;
	while(s < fb + nelem(fb)){
		v = *s++;
		switch(scale){
		case 12: *p++ = v;
		case 11: *p++ = v;
		case 10: *p++ = v;
		case 9: *p++ = v;
		case 8: *p++ = v;
		case 7: *p++ = v;
		case 6: *p++ = v;
		case 5: *p++ = v;
		case 4: *p++ = v;
		case 3: *p++ = v;
		case 2: *p++ = v; *p++ = v;
		}
	}
}

void
drawfb(void)
{
	uchar *p;
	Rectangle r;

	if(scale == 1){
		loadimage(fbs, fbs->r, (uchar*)fb, fbsz);
		draw(screen, fbsr, fbs, nil, ZP);
	}else{
		drawscaled();
		p = (uchar*)fbsbuf;
		r = fbsr;
		while(r.min.y < fbsr.max.y){
			r.max.y = r.min.y + scale;
			p += loadimage(fbs, fbs->r, p, fbsz / Vheight);
			draw(screen, r, fbs, nil, ZP);
			r.min.y = r.max.y;
		}
	}
	flushimage(display, 1);
}

void
resetfb(int paint)
{
	Point o, p;

	scale = min(Dx(screen->r) / Vwidth, Dy(screen->r) / Vheight);
	if(scale <= 0)
		scale = 1;
	else if(scale > 12)
		scale = 12;
	o = divpt(addpt(screen->r.min, screen->r.max), 2);
	p = Pt(Vwidth / 2 * scale, Vheight / 2 * scale);
	fbsr = Rpt(subpt(o, p), addpt(o, p));
	fbsz = Vwidth * Vheight * scale * sizeof *fbsbuf;
	freeimage(fbs);
	if((fbs = allocimage(display, Rect(0,0,Vwidth*scale,scale==1? Vheight : 1),
		XRGB32, scale > 1, DBlack)) == nil)
		sysfatal("allocimage: %r");
	free(fbsbuf);
	fbsbuf = nil;
	if(scale != 1)
		fbsbuf = emalloc(fbsz);
	draw(screen, screen->r, display->black, nil, ZP);
	if(paint)
		drawfb();
	else
		flushimage(display, 1);
}

void
initfb(void)
{
	if(initdraw(nil, nil, "dporg") < 0)
		sysfatal("initdraw: %r");
	loadpics();
	resetfb(0);
}
