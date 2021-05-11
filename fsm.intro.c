#include <u.h>
#include <libc.h>
#include <thread.h>
#include "dat.h"
#include "fns.h"

enum{
	Ttext = 25,
	Tbg1scroll = 157,
	Tbg2scroll = 457,
	TshipΔx = 142,
	TshipΔy = 333,
	Tblink = 500,
	Tshiptravel = 10000,
};

static char *curstr, *prompt;
static int stri;
static int steptc;
static int canvmidx, canvmidy, promptΔx;
static int scrollΔtc, scroll2Δtc, txtΔt;

static void
drawbg(void)
{
	scrollpic(&pics[PCspace], tc / scrollΔtc);
	drawpic(canvmidx, canvmidy, &canvas);
}

static void
drawprompt(void)
{
	Pic *pp;

	pp = &pics[PCcur];
	drawstr(canvmidx + canvas.w - 4 - strlen(prompt) * Vfntw,
		canvmidy + canvas.h - Vfnth, prompt);
	drawpic(Vcenterx + promptΔx - pp->w,
		canvmidy + canvas.h - 2 - pp->h, pp);
}

static void
drawtarget(void)
{
	int x, y;

	x = Vcenterx + tc / TshipΔx;
	y = Vcentery + 22 - tc / TshipΔy;
	drawline(x, y - 1, 10, 1, DShip);
	drawline(x + 4, canvmidy, 1, y - canvmidy - 1, DShip);
	drawline(x, y + 9, 10, 1, DShip);
	drawline(x + 4, y + 9, 1, canvmidy + canvas.h - y - 9, DShip);
	if(x - 1 > canvmidx)
		drawline(x - 1, y, 1, 10, DShip);
	drawline(canvmidx, y + 4, x - 1 - canvmidx + canvas.w, 1, DShip);
	drawline(x + 9, y, 1, 9, DShip);
	drawline(x + 9, y + 4, canvmidx + canvas.w - x - 9, 1, DShip);
}

static void
drawship(void)
{
	drawpic(Vcenterx + tc / TshipΔx, Vh / 2 + 22 - tc / TshipΔy, &pics[PCship]);
}

static void
drawplanets(void)
{
	drawpic(canvmidx, canvmidy, &pics[PCplanets]);
}

static void
drawgrid(void)
{
	scrollpic(&pics[PCgrid], tc / scroll2Δtc);
	drawpic(canvmidx, canvmidy, &canvas);
}

static void
drawintrostr(void)
{
	if(stri < strlen(curstr))
		stri = tc / txtΔt;
	drawsubstr(canvmidx, canvmidy, curstr, curstr+stri);
}

static void
intro3step(void)
{
	int tc₁, tc₂;

	if(tc > Tshiptravel){
		input(Kfire);
		return;
	}
	if(tc < steptc)
		return;
	drawbg();
	drawgrid();
	drawplanets();
	drawship();
	if((tc / Tblink & 1) == 0)
		drawtarget();
	drawfb();
	tc₁ = (tc / scrollΔtc + 1) * scrollΔtc;
	tc₂ = (tc / scroll2Δtc + 1) * scroll2Δtc;
	tc₁ = min(tc₁, tc₂);
	tc₂ = (tc / TshipΔx + 1) * TshipΔx;
	tc₁ = min(tc₁, tc₂);
	tc₂ = (tc / TshipΔy + 1) * TshipΔy;
	tc₁ = min(tc₁, tc₂);
	tc₂ = (tc / Tblink + 1) * Tblink;
	steptc = min(tc₁, tc₂);
}

static void
scrollstep(void)
{
	int tc₁, tc₂;

	if(tc < steptc)
		return;
	drawbg();
	drawintrostr();
	drawprompt();
	drawfb();
	tc₁ = (tc / scrollΔtc + 1) * scrollΔtc;
	tc₂ = stri < strlen(curstr) ? (tc / txtΔt + 1) * txtΔt : scrollΔtc;
	steptc = min(tc₁, tc₂);
}

static void
intro4step(void)
{
	scrollstep();
}

static void
intro2step(void)
{
	scrollstep();
}

static void
intro1step(void)
{
	scrollstep();
}

static void
introkey(Rune k)
{
	if(k != Kfire)
		return;
	if(stri < strlen(curstr))
		stri = strlen(curstr);
	else if(step == intro1step){
		setfsm(intro2step, introkey);
		curstr = basestr[BSintro2];
		stri = 0;
		steptc = 0;
	}else if(step == intro2step){
		setfsm(intro3step, introkey);
		scrollΔtc = Tbg2scroll;
		scroll2Δtc = Tbg1scroll;
		steptc = 0;
	}else if(step == intro3step){
		setfsm(intro4step, introkey);
		curstr = basestr[BSintro3];
		prompt = basestr[BScontinue];
		promptΔx = 8 - 4;
		scrollΔtc = Tbg1scroll;
		stri = 0;
		steptc = 0;
	}else
		enterloadmap();
}

void
enterintro(void)
{
	canvas.w = pics[PCplanets].w;
	canvas.h = pics[PCplanets].h;
	canvmidx = Vcenterx - canvas.w / 2;
	canvmidy = Vcentery - canvas.h / 2;
	setfsm(intro1step, introkey);
	curstr = basestr[BSintro1];
	prompt = basestr[BSmore];
	promptΔx = 36 - 30;
	scrollΔtc = Tbg1scroll;
	txtΔt = Ttext;
	steptc = 0;
}
