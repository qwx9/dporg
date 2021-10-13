#include <u.h>
#include <libc.h>
#include <draw.h>
#include "dat.h"
#include "fns.h"

void (*step)(void);
void (*input)(Rune);

vlong tc;
static vlong t0;

enum{
	Te9 = 1000000000,
	Te6 = 1000000,
	Hz = 1000,
	TΔ = Te9 / Hz,
};

int
advclock(void)
{
	int Δtc;
	vlong t;

	t = nsec();
	Δtc = (t - t0) / TΔ;
	if(Δtc <= 0)
		Δtc = 1;
	tc += Δtc;
	t0 += Δtc * TΔ;
	return Δtc;
}

void
setfsm(void (*stepfn)(void), void (*inputfn)(Rune), int clear)
{
	step = stepfn;
	input = inputfn;
	t0 = nsec();
	if(clear)
		tc = 0;
}

void
initfsm(void)
{
	srand(time(nil));
	enterintro();
}
