


#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu.h"
#include "mem.h"
#include "lcd.h"
#include "rc.h"
#include "rtc.h"
#include "sys.h"
#include "sound.h"
#include "cpu.h"


static int framelen = 16743;
static int framecount;

rcvar_t emu_exports[] =
{
	RCV_INT("framelen", &framelen),
	RCV_INT("framecount", &framecount),
	RCV_END
};







void emu_init()
{
	
}


/*
 * emu_reset is called to initialize the state of the emulated
 * system. It should set cpu registers, hardware registers, etc. to
 * their appropriate values at powerup time.
 */

void emu_reset()
{
ets_printf("%s\n", __func__);
	hw_reset();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	lcd_reset();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	cpu_reset();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	mbc_reset();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	sound_reset();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
}





void emu_step()
{
	cpu_emulate(cpu.lcdc);
}



/* This mess needs to be moved to another module; it's just here to
 * make things work in the mean time. */

void *sys_timer();

void emu_run()
{
	void *timer = sys_timer();
	int delay;

ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	vid_begin();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	lcd_begin();
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
	for (;;)
	{
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
		ets_printf("step\n");
		cpu_emulate(2280);
ets_printf("%s: rmap=%p\n", __func__, mbc.rmap[0]);
		ets_printf("stepped R_LY=%d\n", R_LY);
		while (R_LY > 0 && R_LY < 144)
			emu_step();
		
		vid_end();
		rtc_tick();
		sound_mix();
		if (!pcm_submit())
		{
			delay = framelen - sys_elapsed(timer);
			sys_sleep(delay);
			sys_elapsed(timer);
		}
		doevents();
		vid_begin();
		if (framecount) { if (!--framecount) die("finished\n"); }
		
		if (!(R_LCDC & 0x80))
			cpu_emulate(32832);
		
		while (R_LY > 0) /* wait for next frame */
			emu_step();
	}
}
