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
static Image *fbs, *bgcol;
static u32int *fbsbuf;

Image *
eallocimage(Rectangle r, int repl, ulong col)
{
	Image *i;

	if((i = allocimage(display, r, XRGB32, repl, col)) == nil)
		sysfatal("allocimage: %r");
	return i;
}

void
scrollpic(Pic *pp, int Δx)
{
	int h, x, Δs;
	u32int *s, *d, *e;

	d = canvas.p;
	Δx %= pp->w;
	s = pp->p + Δx;
	x = (canvas.w + Δx) - pp->w;
	Δs = pp->w - canvas.w;
	if(x > 0)
		Δs += pp->w;
	for(h=0; h<canvas.h; h++){
		for(x=Δx, e=d+canvas.w; d<e; d++, s++){
			if(*s >> 24)
				*d = *s;
			if(x++ == pp->w){
				s -= pp->w;
				x = 0;
			}
		}
		s += Δs;
	}
}

void
drawpic(int x, int y, Pic *pp)
{
	int w, pw;
	u32int *d, *s, *e;

	assert(x < Vw && y < Vh);
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
	int c, w, h, px, py;

	w = dfont[0].w;
	h = dfont[0].h;
	x += Vfntspc;
	for(px=x, py=y; s<e; s++){
		c = *s;
		if(c == '\n'){
			px = x;
			py += h;
			if(py >= Vh - h)
				sysfatal("drawsubstr: drawing string past screen: %s at %d,%d", s, x, y);
			continue;
		}else if(c < fontmap[0])
			goto skip;
		/* FIXME: characters >128 */
		c -= fontmap[0];
		if(c > nglyph)
			sysfatal("drawsubstr: invalid glyph index %d (nglyph %d)", c, nglyph);
		drawpic(px, py, &dfont[c]);
	skip:
		px += w;
		if(px >= Vw - w)
			sysfatal("drawsubstr: drawing string past screen: %s at %d,%d", s, x, y);
	}
}

void
drawstr(int x, int y, char *s)
{
	drawsubstr(x, y, s, s+strlen(s));
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
	fbs = eallocimage(Rect(0,0,Vw*scale,scale==1? Vfullh : 1), scale > 1, DBlack);
	free(fbsbuf);
	fbsbuf = nil;
	if(scale != 1)
		fbsbuf = emalloc(fbsz);
	draw(screen, screen->r, bgcol, nil, ZP);
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
	bgcol = eallocimage(Rect(0,0,1,1), 1, 0xccccccff);
	resetfb(0);
}
