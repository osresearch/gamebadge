/* Hacks to get gnuboy to compile */
#include "rc.h"
#include "pcm.h"
#include "fb.h"
#include "hw.h"
#include "sys.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_fb.h"
#include "badge/badge_input.h"
#include "badge/badge_button.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

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
	uint32_t button = badge_input_get_event(1);
	//if (button == 0) return;

	uint32_t buttons = badge_input_button_state;
	static uint32_t old_buttons;
	uint32_t delta = buttons ^ old_buttons;

	if (delta == 0)
		return;

	ets_printf("%d %08x\n", button, buttons, delta);

	if (delta & (BADGE_BUTTON_UP << 1))
		pad_set(PAD_UP, buttons & BADGE_BUTTON_UP);
	if (delta & (BADGE_BUTTON_DOWN << 1))
		pad_set(PAD_DOWN, buttons & BADGE_BUTTON_DOWN);
	if (delta & (BADGE_BUTTON_LEFT << 1))
		pad_set(PAD_LEFT, buttons & BADGE_BUTTON_LEFT);
	if (delta & (BADGE_BUTTON_RIGHT << 1))
		pad_set(PAD_RIGHT, buttons & BADGE_BUTTON_RIGHT);
	if (delta & (BADGE_BUTTON_START << 1))
		pad_set(PAD_START, buttons & BADGE_BUTTON_START);
	if (delta & (BADGE_BUTTON_SELECT << 1))
		pad_set(PAD_SELECT, buttons & BADGE_BUTTON_SELECT);
	if (delta & (BADGE_BUTTON_A << 1))
		pad_set(PAD_A, buttons & BADGE_BUTTON_A);
	if (delta & (BADGE_BUTTON_B << 1))
		pad_set(PAD_B, buttons & BADGE_BUTTON_B);

	old_buttons = badge_input_button_state;
	pad_refresh();
}

#define GB_WIDTH 160
#define GB_HEIGHT 144

static uint8_t * fb_ram;
static SemaphoreHandle_t fb_mutex;

void fb_draw_task(void * arg)
{
	(void) arg;

	while(1)
	{
		if (!xSemaphoreTake(fb_mutex, 1000))
		{
			ets_printf("waiting for frame\n");
			continue;
		}
		
		// skip the first few lines and center on the display
		badge_eink_display(
			fb_ram - (BADGE_EINK_WIDTH - GB_WIDTH)/2,
			//DISPLAY_FLAG_LUT(3)
			DISPLAY_FLAG_GREYSCALE | DISPLAY_FLAG_LUT(2)
		);

		fb.enabled = 1;
	}
}


void vid_init()
{
	fb_ram = calloc(BADGE_EINK_WIDTH*GB_HEIGHT, 1);
	if (!fb_ram)
		die("fb alloc failed\n");

/*
	fb_mono = calloc(BADGE_EINK_WIDTH*BADGE_EINK_HEIGHT/8, 1);
	if (!fb_mono)
		die("mono alloc failed\n");
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

	// start with black.
	badge_eink_display_one_layer(NULL, DISPLAY_FLAG_FULL_UPDATE);

	// setup our redraw task
	fb_mutex = xSemaphoreCreateMutex();
	xSemaphoreTake(fb_mutex, 0);

	BaseType_t xReturned;
	TaskHandle_t xHandle = NULL;

	/* Create the task, storing the handle. */
	xReturned = xTaskCreate(
		fb_draw_task,       /* Function that implements the task. */
		"eink",          /* Text name for the task. */
		8192,      /* Stack size in words, not bytes. */
		NULL,    /* Parameter passed into the task. */
		tskIDLE_PRIORITY,/* Priority at which the task is created. */
		&xHandle      /* Used to pass out the created task's handle. */
	);

}

void vid_begin()
{
	//ets_printf("begin\n");
}

void vid_end()
{
	if(!fb.enabled)
		return;

	static int framenum;
	ets_printf("frame %d enabled=%d\n", framenum++, fb.enabled);

	// if it is all zero, don't display
	int non_zero = 0;
	for(int i = 0 ; i < GB_WIDTH * GB_HEIGHT ; i++)
	{
		if (fb_ram[i] == 0)
			continue;
		non_zero++;
		break;
	}
	if (non_zero == 0 || framenum < 16)
		return;

/*
	for(int i = 0 ; i < 128 ; i++)
	{
		ets_printf("%02x", fb_ram[i + i * BADGE_EINK_WIDTH]);
		if (i % 32 == 31)
			ets_printf("\n");
	}
*/

	// mark that the fb is in use and wake the drawing task
	fb.enabled = 0;
	xSemaphoreGive(fb_mutex);


/*
	// pack the fb into a single bit per pixel
	for(int y = 0 ; y < GB_HEIGHT ; y++)
	{
		for(int x = 0 ; x < GB_WIDTH ; x += 8)
		{
			uint8_t b = 0;
			for(int xp = 0 ; xp < 8 ; xp++)
			{
				uint8_t p = fb_ram[x + xp + y * GB_WIDTH];
				if (p != 0xFF)
					b = (b << 1) | 1;
			}
			fb_mono[(x + y * BADGE_EINK_WIDTH)/8] = b;
		}
	}
*/

/*
	// skip the first few lines and center on the display
	badge_eink_display(
		fb_ram - (BADGE_EINK_WIDTH - GB_WIDTH)/2,
		//DISPLAY_FLAG_LUT(3)
		DISPLAY_FLAG_GREYSCALE | DISPLAY_FLAG_LUT(3)
	);
	//ets_printf("end\n");
*/
}

