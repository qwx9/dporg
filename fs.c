#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

s32int sintab[256];
char **basestr;
int nfontmap;
uchar *fontmap;
int nglyph;
Pic *dfont;
int nwalls;
Wall *walls;
int nsprites;
Sprite *sprites;

static Biobuf *
eopen(char *s, int mode)
{
	Biobuf *bf;

	if((bf = Bopen(s, mode)) == nil)
		sysfatal("Bopen: %r");
	Blethal(bf, nil);
	return bf;
}

static long
eread(Biobuf *bf, void *buf, long n)
{
	if(Bread(bf, buf, n) != n)
		sysfatal("ebread: short read: %r");
	return n;
}

static vlong
bsize(Biobuf *bf)
{
	vlong n;
	Dir *d;

	d = dirfstat(Bfildes(bf));
	if(d == nil)
		sysfatal("bstat: %r");
	n = d->length;
	free(d);
	return n;
}

static u8int
get8(Biobuf *bf)
{
	u8int v;

	eread(bf, &v, 1);
	return v;
}

static u16int
get16(Biobuf *bf)
{
	u16int v;

	v = get8(bf);
	return v | get8(bf) << 8;
}

static u32int
get24(Biobuf *bf)
{
	u32int v;

	v = get16(bf);
	return v | get8(bf) << 16;
}

static u32int
get32(Biobuf *bf)
{
	u32int v;

	v = get16(bf);
	return v | get16(bf) << 16;
}

static void
loadpic(char *name, Pic *pic)
{
	int fd, n, m, dx, dy;
	uchar *b, *s;
	u32int *p;
	Image *i;

	if((fd = open(name, OREAD)) < 0)
		sysfatal("loadpic: %r");
	if((i = readimage(display, fd, 0)) == nil)
		sysfatal("readimage: %r");
	close(fd);
	if(i->chan != RGBA32)
		sysfatal("loadpic %s: inappropriate image format", name);
	dx = Dx(i->r);
	dy = Dy(i->r);
	n = dx * dy;
	if(dx * dy > Vw * Vfullh)
		sysfatal("loadpic %s: inappropriate image size", name);
	p = emalloc(n * sizeof *p);
	pic->p = p;
	pic->w = dx;
	pic->h = dy;
	m = i->depth / 8;
	b = emalloc(n * m);
	unloadimage(i, i->r, b, n * m);
	freeimage(i);
	s = b;
	while(n-- > 0){
		*p++ = s[0] << 24 | s[3] << 16 | s[2] << 8 | s[1];
		s += m;
	}
	free(b);
}

static void
loadfont(void)
{
	int i, n, nx, ny;
	u32int *d, *s;
	Pic pic, *pp;

	loadpic("a.bit", &pic);
	nx = pic.w / Vfntpicw;
	ny = pic.h / Vfnth;
	if(pic.w % nx != 0 || pic.h % ny != 0)
		sysfatal("loadfont: invalid font pic");
	nglyph = nx * ny;
	dfont = emalloc(nglyph * sizeof *dfont);
	for(i=0, pp=dfont; pp<dfont+nglyph; pp++, i++){
		pp->w = Vfntw;
		pp->h = Vfnth;
		pp->p = emalloc(pp->w * pp->h * sizeof *pp->p);
		d = pp->p;
		s = pic.p + i / nx * pic.w * pp->h + i % nx * Vfntpicw + Vfntspc;
		for(n=0; n<pp->h; n++){
			memcpy(d, s, pp->w * sizeof *d);
			d += pp->w;
			s += pic.w;
		}
	}
	free(pic.p);
}

void
loadpics(void)
{
	loadpic("b.bit", pics + PCarrow);
	loadpic("c.bit", pics + PCspace);
	loadpic("d.bit", pics + PCgrid);
	loadpic("e.bit", pics + PCplanets);
	loadpic("f.bit", pics + PCship);
	loadpic("k.bit", pics + PChud);
	loadpic("l.bit", pics + PCface);
	loadpic("m.bit", pics + PCammo);
	loadpic("n.bit", pics + PChit);
	loadpic("o.bit", pics + PCdir);
	loadpic("p.bit", pics + PCcur);
	loadpic("q.bit", pics + PCscroll);
	loadpic("r.bit", pics + PCgibs);
	loadfont();
	canvas.p = emalloc(Vw * Vfullh * sizeof *canvas.p);
	canvas.w = Vw;
	canvas.h = Vfullh;
}

static void
loadpal(void)
{
	int n;
	u8int r, g, b;
	u32int *p;
	Biobuf *bf;

	bf = eopen("palettes.bin", OREAD);
	npal = get32(bf) / 2;
	if(npal % Npalcol != 0)
		sysfatal("loadpal: invalid palette size %d\n", npal);
	pal = emalloc(npal * sizeof *pal);
	for(p=pal; p<pal+npal; p++){
		n = get16(bf);
		r = n & 0x1f;
		r = r << 3 | r >> 2;
		g = n >> 5 & 0x3f;
		g = g << 2 | g >> 4;
		b = n >> 11 & 0x1f;
		b = b << 3 | b >> 2;
		*p = 0xff << 24 | r << 16 | g << 8 | b;
	}
	npal /= Npalcol;
	Bterm(bf);
}

static char **
loadstr(char *name, int *nel)
{
	int n;
	char **s, **p, **e, *q;
	Biobuf *bf;

	bf = eopen(name, OREAD);
	n = get16(bf);
	s = emalloc(n * sizeof *p);
	e = s + n;
	*nel = n;
	for(p=s; p<e; p++){
		n = get16(bf);
		*p = emalloc(n + 1);
		eread(bf, *p, n);
		/* FIXME: not always? */
		q = *p;
		while((q = strchr(q, '|')) != nil)
			*q = '\n';
	}
	Bterm(bf);
	return s;
}

static void
unpackspr(Sprite *s, Biobuf *btex, Biobuf *bshp)
{
	int n, B, b, i, v;
	u32int *p;

	v = -1;
	for(i=0; i<s->w; i++){
		p = s->p + i;
		n = s->h;
		while(n > 0){
			B = n > 8 ? 8 : n;
			n -= B;
			b = get8(bshp);
			while(B > 0){
				if(b & 1){
					if(v >= 0){
						*p = v & 15;
						v = -1;
					}else{
						v = get8(btex);
						*p = v & 15;
						v >>= 4;
					}
				}else
					*p = -1;
				p += s->w;
				b >>= 1;
				B--;
			}
		}
	}
}

static void
dumpbwwalls(Wall *wbuf, int nbuf)
{
	int n, fd;
	char name[32], c[9];
	Wall *w;

	for(w=wbuf; w<wbuf+nbuf; w++){
		snprint(name, sizeof name, "wall%02zd.bit", w-walls);
		if((fd = create(name, OWRITE, 0644)) < 0)
			sysfatal("dumpbwwalls: %r");
		fprint(fd, "%11s %11d %11d %11d %11d ",
			chantostr(c, GREY8), 0, 0, Wallsz, Wallsz);
		for(n=0; n<nelem(w->p); n++){
			c[0] = w->p[n] & 0xff;
			write(fd, c, 1);
		}
		close(fd);
	}
}

static void
dumpbwspr(Sprite *sbuf, int nbuf)
{
	int n, fd;
	char name[32], c[9];
	Sprite *s;

	for(s=sbuf; s<sbuf+nbuf; s++){
		snprint(name, sizeof name, "spr%02zd.bit", s-sbuf);
		if((fd = create(name, OWRITE, 0644)) < 0)
			sysfatal("dumpbwspr: %r");
		fprint(fd, "%11s %11d %11d %11d %11d ",
			chantostr(c, GREY8), 0, 0, s->w, s->h);
		for(n=0; n<s->w*s->h; n++){
			c[0] = 96 * (s->p[n] & 0xff) / 16;
			write(fd, c, 1);
		}
		close(fd);
	}
}

static void
dumpspr(Sprite *sbuf, int nbuf)
{
	int fd;
	char name[32], c[9];
	Sprite *s;

	for(s=sbuf; s<sbuf+nbuf; s++){
		snprint(name, sizeof name, "spr%02zd.bit", s-sbuf);
		if((fd = create(name, OWRITE, 0644)) < 0)
			sysfatal("dumpspr: %r");
		fprint(fd, "%11s %11d %11d %11d %11d ",
			chantostr(c, ARGB32), 0, 0, s->w, s->h);
		write(fd, s->p, s->w * s->h * sizeof *s->p);
		close(fd);
	}
}

static void
dumppal(void)
{
	int fd;
	char name[32], c[9];

	snprint(name, sizeof name, "pal.bit");
	if((fd = create(name, OWRITE, 0644)) < 0)
		sysfatal("dumppal: %r");
	fprint(fd, "%11s %11d %11d %11d %11d ", chantostr(c, ARGB32), 0, 0, 16, npal);
	write(fd, pal, npal * Npalcol * sizeof *pal);
	close(fd);
}

static int
loadblankspr(Sprite **sbuf, int *nsbuf, int **shpofs)
{
	int nbuf, nspan, spansz, bshpsz, bsize, ofs, *ofsbuf, *o;
	Biobuf *btex, *bshp;
	Sprite *s, *buf;

	btex = eopen("stexels.bin", OREAD);
	get32(btex);
	bshp = eopen("bitshapes.bin", OREAD);
	bshpsz = get32(bshp);
	nbuf = 0;
	buf = nil;
	s = nil;
	ofsbuf = nil;
	o = nil;
	ofs = 0;
	while(bshpsz > 0){
		if(s >= buf + nbuf){
			buf = erealloc(buf, (nbuf+32) * sizeof *buf,
				nbuf * sizeof *buf);
			ofsbuf = erealloc(ofsbuf, (nbuf+32) * sizeof *ofsbuf,
				nbuf * sizeof *ofsbuf);
			s = buf + nbuf;
			o = ofsbuf + nbuf;
			nbuf += 32;
		}
		get16(bshp);
		get16(bshp);
		bsize = get16(bshp) + 8;
		get16(bshp);
		s->r.min.x = get8(bshp);
		s->r.max.x = get8(bshp);
		s->r.min.y = get8(bshp);
		s->r.max.y = get8(bshp);
		nspan = Dx(s->r) + 1;
		spansz = Dy(s->r) + 1;
		s->w = nspan;
		s->h = spansz;
		s->p = emalloc(s->w * s->h * sizeof *s->p);
		unpackspr(s, btex, bshp);
		s++;
		*o++ = ofs;
		ofs += bsize;
		bshpsz -= bsize;
	}
	if(bshpsz != 0){
		werrstr("sprite data and shape mismatch by %d bytes", bshpsz);
		return -1;
	}
	*sbuf = buf;
	*nsbuf = s - buf;
	*shpofs = ofsbuf;
	Bterm(btex);
	Bterm(bshp);
	return 0;
}

static int
gencolorspr(Sprite *sbuf, int nbuf, int *shpofs)
{
	int *shp;
	u32int n, ofs, palofs, *pp, *bp, *p;
	Biobuf *bf;
	Sprite *s, *bw;

	bf = eopen("mappings.bin", OREAD);
	ofs = get32(bf) * 2 * sizeof(u32int);
	n = get32(bf);
	get32(bf);
	get32(bf);
	Bseek(bf, ofs, 1);
	sprites = emalloc(n * sizeof *sprites);
	nsprites = n;
	for(s=sprites; s<sprites+nsprites; s++){
		ofs = get32(bf);
		palofs = get32(bf);
		if(palofs % Npalcol != 0 || palofs / Npalcol >= npal){
			werrstr("gencolorspr: invalid palette index %ud", palofs);
			return -1;
		}
		for(shp=shpofs; shp<shpofs+nbuf; shp++)
			if(*shp == ofs)
				break;
		if(shp >= shpofs+nbuf){
			werrstr("gencolorspr: invalid bitshapes offset %ud", ofs);
			return -1;
		}
		bw = sbuf + (shp - shpofs);
		pp = pal + palofs;
		memcpy(s, bw, sizeof *s);
		s->p = emalloc(s->w * s->h * sizeof *s->p);
		for(p=s->p, bp=bw->p; p<s->p+s->w*s->h; p++, bp++){
			n = *bp;
			if(n == -1UL)
				*p = 0;
			else if(n > Npalcol){
				werrstr("gencolor: invalid palette color index %ud", n);
				return -1;
			}else
				*p = pp[n];
		}
	}
	Bterm(bf);
	return 0;
}

static void
loadsprites(void)
{
	int nbuf, *shpofs;
	Sprite *sbuf, *s;

	sbuf = nil;
	nbuf = 0;
	shpofs = nil;
	if(loadblankspr(&sbuf, &nbuf, &shpofs) < 0
	|| gencolorspr(sbuf, nbuf, shpofs) < 0)
		sysfatal("loadsprites: %r");
	for(s=sbuf; s<sbuf+nbuf; s++)
		free(s->p);
	free(sbuf);
	free(shpofs);
}

static void
loadwalls(void)
{
	int i, j, n, v;
	Biobuf *bf;
	Wall *w, *we;
	u32int *p;

	bf = eopen("wtexels.bin", OREAD);
	n = get32(bf) / (Wallsz * Wtexelsz);
	walls = emalloc(n * sizeof *walls);
	nwalls = n;
	for(w=walls, we=w+n; w<we; w++){
		for(i=0; i<Wallsz; i++){
			p = w->p + i;
			for(j=0; j<Wtexelsz; j++){
				v = get8(bf);
				*p = v & 15;
				p += Wallsz;
				*p = v >> 4 & 15;
				p += Wallsz;
			}
		}
	}
	Bterm(bf);
}

static void
loadfontmap(void)
{
	int n;
	Biobuf *bf;

	bf = eopen("a.map", OREAD);
	n = bsize(bf);
	fontmap = emalloc(n * sizeof *fontmap);
	eread(bf, fontmap, n);
	nfontmap = n;
	Bterm(bf);
}

static void
loadbasestr(void)
{
	int nel;

	basestr = loadstr("base.str", &nel);
	if(nel != BSend)
		sysfatal("loadbasestr: inconsistent base.str file: entries %d not %d", nel, BSend);
}

static void
loadsintab(void)
{
	Biobuf *bf;
	s32int *s;

	bf = eopen("sintable.bin", OREAD);
	for(s=sintab; s<sintab+nelem(sintab); s++)
		*s = get32(bf);
	Bterm(bf);
}

void
initfs(void)
{
	rfork(RFNAMEG);
	if(bind(".", prefix, MBEFORE|MCREATE) == -1 || chdir(prefix) < 0)
		fprint(2, "initfs: %r\n");
	loadsintab();
	loadpal();
	loadbasestr();
	loadfontmap();
	loadwalls();
	loadsprites();
}
