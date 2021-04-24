#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dat.h"
#include "fns.h"

s32int sintab[256];
int npal;
u32int *pal;

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
readpal(void)
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
		*p = r << 16 | g << 8 | b;
	}
	Bterm(bf);
}

static void
readsintab(void)
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
	readsintab();
	readpal();
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
// .bit (r8g8b8a8)
// + old project code
