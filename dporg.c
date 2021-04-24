#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include "dat.h"
#include "fns.h"

char *prefix = "/sys/games/lib/dporg";

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
	fprint(2, "usage: %s [-d datadir]\n", argv0);
	threadexits("usage");
}

void
threadmain(int argc, char **argv)
{
	ARGBEGIN{
	case 'd': prefix = EARGF(usage()); break;
	default: usage();
	}ARGEND
	if(initdraw(nil, nil, "dporg") < 0)
		sysfatal("initdraw: %r");
	initfs();
	threadexits(nil);
}
