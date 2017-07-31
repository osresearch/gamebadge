/* Hacks to get gnuboy to compile */
#include "rc.h"
#include "pcm.h"
#include "fb.h"

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


void vid_begin()
{
	ets_printf("begin\n");
}

void vid_end()
{
	ets_printf("end\n");
}

