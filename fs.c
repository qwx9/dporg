#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

s32int sintab[256];

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

void
loadpics(void)
{
	loadpic("a.bit", pics + PCfont);
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
	Bterm(bf);
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
}

// grids: 32/256/2048
// wtexels.bin
// stexels.bin
// bitshapes.bin
// entities.db
// entities.str
// mappings.bin
// map bsp + str (on demand, with shim load gauge)
// a.map
// base.str
// + old project code
