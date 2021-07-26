#include <u.h>
#include <libc.h>
#include <pool.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include "dat.h"
#include "fns.h"

char *prefix = "/sys/games/lib/dporg";

static Keyboardctl *kc;
static Mousectl *mc;

int
max(int a, int b)
{
	return a > b ? a : b;
}

int
min(int a, int b)
{
	return a < b ? a : b;
}

void *
erealloc(void *p, ulong n, ulong oldn)
{
	if((p = realloc(p, n)) == nil)
		sysfatal("realloc: %r");
	setrealloctag(p, getcallerpc(&p));
	if(n > oldn)
		memset((uchar *)p + oldn, 0, n - oldn);
	return p;
}

void *
emalloc(ulong n)
{
	void *p;

	if((p = mallocz(n, 1)) == nil)
		sysfatal("emalloc: %r");
	setmalloctag(p, getcallerpc(&n));
	return p;
}

static void
usage(void)
{
	fprint(2, "usage: %s [-m datadir]\n", argv0);
	threadexits("usage");
}

void
threadmain(int argc, char **argv)
{
	Rune r;
	Mouse mo;

	ARGBEGIN{
	case 'm': prefix = EARGF(usage()); break;
	default: usage();
	}ARGEND
	mainmem->flags |= POOL_PARANOIA | POOL_NOREUSE;
	initfs();
	initfb();
	if((kc = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");
	if((mc = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	srand(time(nil));
	initfsm();
	enum{
		Aresize,
		Amouse,
		Akbd,
		Aend,
	};
	Alt a[] = {
		[Aresize] {mc->resizec, nil, CHANRCV},
		[Amouse] {mc->c, &mc->Mouse, CHANRCV},
		[Akbd] {kc->c, &r, CHANRCV},
		[Aend] {nil, nil, CHANNOBLK},
	};
	for(;;){
		switch(alt(a)){
		case Aresize:
			if(getwindow(display, Refnone) < 0)
				sysfatal("resize failed: %r");
			mo = mc->Mouse;
			resetfb(1);
			break;
		case Amouse:
			if(eqpt(mo.xy, ZP))
				mo = mc->Mouse;
			break;
		case Akbd:
			switch(r){
			case Kdel:
			case 'q': threadexitsall(nil);
			case 'x': r = Kfire; break;
			case Kup: r = K↑; break;
			case Kdown: r = K↓; break;
			case Kleft: r = K←; break;
			case Kright: r = K→; break;
			default: r = -1; break;
			}
			if(r != -1 && input != nil)
				input(r);
			break;
		}
		if(step != nil){
			advclock();
			step();
		}
		a[Aend].op = step == nil ? CHANEND : CHANNOBLK;
		sleep(1);
	}
}
