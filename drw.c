#include <u.h>
#include <libc.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

int npal;
u32int *pal;
Pic pics[PCend], canvas;

static u32int fb[Vw * Vfullh];
static int scale, fbsz;
static Rectangle fbsr;
static Image *fbs;
static u32int *fbsbuf;

void
scrollpic(Pic *pp, int Δx)
{
	int h, w;
	u32int *s, *d;

	d = canvas.p;
	s = pp->p + Δx % pp->w;
	for(h=0; h<canvas.h; h++){
		for(w=0; w<canvas.w; w++){
			if(*s >> 24)
				*d = *s;
			d++, s++;
		}
		s += pp->w - canvas.w;
	}
}

void
drawpic(int x, int y, Pic *pp)
{
	int w, pw;
	u32int *d, *s, *e;

	d = fb + Vw * y + x;
	s = pp->p;
	pw = pp->w;
	e = s + pw * pp->h;
	while(s < e){
		for(w=0; w<pw; w++){
			if(*s >> 24)
				*d = *s;
			d++, s++;
		}
		d += Vw - pw;
	}
}

void
drawsubstr(int x, int y, char *s, char *e)
{
	// FIXME
	USED(x,y,s,e);
	//use a drawsubpic?
}

void
drawstr(int x, int y, char *s)
{
	// FIXME
	USED(x,y,s);
}

void
drawrect(int x, int y, int w, int h, u32int col)
{
	int n;
	u32int *d;

	d = fb + Vw * y + x;
	while(h-- > 0){
		for(n=0; n<w; n++)
			*d++ = col;
		d += Vw - w;
	}
}

void
drawfill(u32int col)
{
	u32int *p;

	for(p=fb; p<fb+nelem(fb); p++)
		*p = col;
}

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
		loadimage(fbs, fbs->r, (uchar*)fb, sizeof fb);
		draw(screen, fbsr, fbs, nil, ZP);
	}else{
		drawscaled();
		p = (uchar*)fbsbuf;
		r = fbsr;
		while(r.min.y < fbsr.max.y){
			r.max.y = r.min.y + scale;
			p += loadimage(fbs, fbs->r, p, fbsz / Vfullh);
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

	scale = min(Dx(screen->r) / Vw, Dy(screen->r) / Vfullh);
	if(scale <= 0)
		scale = 1;
	else if(scale > 12)
		scale = 12;
	o = divpt(addpt(screen->r.min, screen->r.max), 2);
	p = Pt(Vw / 2 * scale, Vfullh / 2 * scale);
	fbsr = Rpt(subpt(o, p), addpt(o, p));
	fbsz = Vw * Vfullh * scale * sizeof *fbsbuf;
	freeimage(fbs);
	if((fbs = allocimage(display, Rect(0,0,Vw*scale,scale==1? Vfullh : 1),
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
