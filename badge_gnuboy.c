/* Hacks to get gnuboy to compile */
#include "rc.h"
#include "pcm.h"
#include "fb.h"
#include "hw.h"
#include "sys.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_lut.h"
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

	if (delta & (1 << BADGE_BUTTON_UP))
		pad_set(PAD_UP, buttons & (1 << BADGE_BUTTON_UP));
	if (delta & (1 << BADGE_BUTTON_DOWN))
		pad_set(PAD_DOWN, buttons & (1 << BADGE_BUTTON_DOWN));
	if (delta & (1 << BADGE_BUTTON_LEFT))
		pad_set(PAD_LEFT, buttons & (1 << BADGE_BUTTON_LEFT));
	if (delta & (1 << BADGE_BUTTON_RIGHT))
		pad_set(PAD_RIGHT, buttons & (1 << BADGE_BUTTON_RIGHT));
	if (delta & (1 << BADGE_BUTTON_START))
		pad_set(PAD_START, buttons & (1 << BADGE_BUTTON_START));
	if (delta & (1 << BADGE_BUTTON_SELECT))
		pad_set(PAD_SELECT, buttons & (1 << BADGE_BUTTON_SELECT));
	if (delta & (1 << BADGE_BUTTON_A))
		pad_set(PAD_A, buttons & (1 << BADGE_BUTTON_A));
	if (delta & (1 << BADGE_BUTTON_B))
		pad_set(PAD_B, buttons & (1 << BADGE_BUTTON_B));

	old_buttons = badge_input_button_state;
	pad_refresh();
}

#define GB_WIDTH 160
#define GB_HEIGHT 144

#undef CONFIG_MONO

static uint8_t * fb_ram;
static uint8_t * fb_mono;
static SemaphoreHandle_t fb_mutex;
static volatile int done_drawing;

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

#ifdef CONFIG_MONO
/*
		struct badge_eink_enty_lut lut[] = {
			{ .length = 64, .voltages = 0x88, },
			{ .length = 0 },
		};
*/
		struct badge_eink_update eink_upd = {
			.lut = BADGE_EINK_LUT_DEFAULT,
			.lut_flags = LUT_FLAG_PARTIAL,
			.reg_0x3a = 26, // dummy lines per gate?
			.reg_0x3b = 0x08, // 62 usec
			.y_start = 0,
			.y_end = GB_WIDTH,
		};
		//badge_eink_write_bitplane(fb_mono, 0, GB_WIDTH);
		badge_eink_update(&eink_upd);
#else
		// copy it into a single-color image
		for(unsigned y = 0 ; y < BADGE_EINK_HEIGHT ; y++)
		{
			for(unsigned x = 0 ; x < GB_WIDTH ; x++)
			{
				uint8_t p = fb_ram[x + y * BADGE_EINK_WIDTH];
				fb_ram[x + y * BADGE_EINK_WIDTH] = p > 0x40 ? 0xFF : 0;
			}
			for(unsigned x = GB_WIDTH ; x < BADGE_EINK_WIDTH ; x++)
				fb_ram[x + y * BADGE_EINK_WIDTH] = 0xFF;
		}

		// black it every few frames
		static unsigned refresh_count;
		if ((refresh_count++ & 0x7F) == 0)
			badge_eink_display_one_layer(NULL, DISPLAY_FLAG_FULL_UPDATE);

		badge_eink_display_one_layer(
			fb_ram - (BADGE_EINK_WIDTH - GB_WIDTH)/2,
			DISPLAY_FLAG_GREYSCALE | DISPLAY_FLAG_LUT(2)
		);
#endif

		done_drawing = 1;
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
	fb.w = GB_WIDTH;
	fb.h = GB_HEIGHT;
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

	if (!xReturned)
		die("unable to create fb thread\n");
}

void vid_begin()
{
	//ets_printf("begin\n");
}

void vid_end()
{
	if(!fb.enabled)
	{
		if (done_drawing)
			fb.enabled = 1;
		return;
	}

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
	done_drawing = fb.enabled = 0;
	xSemaphoreGive(fb_mutex);
}

