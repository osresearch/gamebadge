/* Hacks to get gnuboy to compile */
#include "rc.h"
#include "pcm.h"
#include "fb.h"
#include "sys.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_fb.h"

rcvar_t vid_exports[] = { RCV_END };
rcvar_t joy_exports[] = { RCV_END };
rcvar_t pcm_exports[] = { RCV_END };

void vid_setpal(int i, int r, int g, int b)
{
	// should set the pallete; ignore for now
}

struct pcm pcm;

struct fb fb;


/*
void sound_write(byte r, byte b){}
byte sound_read(byte r){ return 0; }
void sound_dirty() {}
void sound_off() {}
void sound_reset() {}
void sound_mix() {}
*/

// never have any audio to process
int pcm_submit() { return 0; }


// timers are just uint32_t
void * sys_timer()
{
	static uint32_t timer;
	return &timer;
}


void doevents()
{
}


static uint8_t * fb_ram;


void vid_init()
{
	fb_ram = calloc(BADGE_EINK_WIDTH*144, 1);
	if (!fb_ram)
		die("fb alloc failed\n");
/*
	fb.w = BADGE_EINK_WIDTH;
	fb.h = BADGE_EINK_HEIGHT;
	fb.pitch = BADGE_EINK_WIDTH;
*/
	fb.w = 160;
	fb.h = 144;
	fb.pitch = BADGE_EINK_WIDTH;

	fb.ptr = fb_ram;
	fb.pelsize = 1;
	fb.indexed = 0;

	// we have no color, but we pack r/g/b into 8 bits
	fb.cc[0].r = 5;
	fb.cc[1].r = 5;
	fb.cc[2].r = 6;

	fb.cc[0].l = 0;
	fb.cc[1].l = 3;
	fb.cc[2].l = 6;
	fb.enabled = 1;
	fb.dirty = 1;
}

void vid_begin()
{
	//ets_printf("begin\n");
}

void vid_end()
{
	static int framenum;
	ets_printf("frame %d\n", framenum++);
	// if it is all zero, don't display
	int non_zero = 0;
	for(int i = 0 ; i < BADGE_EINK_WIDTH * BADGE_EINK_HEIGHT ; i++)
	{
		if (fb_ram[i] == 0)
			continue;
		non_zero++;
		break;
	}
	if (non_zero == 0 || framenum < 100)
		return;

	for(int i = 0 ; i < 128 ; i++)
	{
		ets_printf("%02x", fb_ram[i + i * BADGE_EINK_WIDTH]);
		if (i % 32 == 31)
			ets_printf("\n");
	}

	badge_eink_display(
		fb_ram,
		DISPLAY_FLAG_GREYSCALE | DISPLAY_FLAG_LUT(2)
	);
	//ets_printf("end\n");
}

