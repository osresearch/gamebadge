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

// What is "black" versus "white" on the mono display?
#define THRESHOLD 0x60


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

	//ets_printf("%d %08x\n", button, buttons, delta);

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


static uint8_t * fb_ram;
static SemaphoreHandle_t fb_mutex;
static volatile int done_drawing;


static void eink_update(
	const uint8_t * img,
	unsigned flags,
	int y_start,
	int y_end
)
{
#if 1
	int lut_mode = 
		(flags >> DISPLAY_FLAG_LUT_BIT) & ((1 << DISPLAY_FLAG_LUT_SIZE)-1);

	int lut_flags = 0;

	uint32_t * buf = badge_eink_tmpbuf;
	badge_eink_create_bitplane(img, buf, 0x80, flags);

	// old image not known; do full update
	// black it every few frames
	static unsigned refresh_count;
	if ((refresh_count++ & 0x7F) == 0)
	{
		badge_eink_display(img, DISPLAY_FLAG_8BITPIXEL | DISPLAY_FLAG_FULL_UPDATE);
		return;
	}

	lut_flags |= LUT_FLAG_PARTIAL;

	struct badge_eink_update eink_upd = {
		.lut       = lut_mode > 0 ? lut_mode - 1 : BADGE_EINK_LUT_DEFAULT,
		.lut_flags = lut_flags,
		.reg_0x3a  = 26,   // 26 dummy lines per gate
		.reg_0x3b  = 0x08, // 62us per line
		.y_start   = y_start,
		.y_end     = y_end,
	};

	badge_eink_update(buf, &eink_upd);
#else
	// black it every few frames
	static unsigned refresh_count;
	if ((refresh_count++ & 0x7F) == 0)
		badge_eink_display(NULL, DISPLAY_FLAG_FULL_UPDATE);

	badge_eink_display_greyscale(img, flags, 8);
#endif
}

void fb_draw_task(void * arg)
{
	(void) arg;
	unsigned start = sys_micros();
	unsigned count = 0;

	while(1)
	{
		if (!xSemaphoreTake(fb_mutex, 1000))
		{
			ets_printf("waiting for frame\n");
			continue;
		}

		for(unsigned y = 0 ; y < BADGE_EINK_HEIGHT ; y++)
		{
			// copy it into a single-color image
			for(unsigned x = 0 ; x < GB_WIDTH ; x++)
			{
				uint8_t p = fb_ram[x + y * BADGE_EINK_WIDTH];
				fb_ram[x + y * BADGE_EINK_WIDTH] = p > THRESHOLD ? 0xFF : 0;
			}

			// checkerboard the background;
			// should we shift it every so often?
			for(unsigned x = GB_WIDTH ; x < BADGE_EINK_WIDTH ; x++)
			{
				fb_ram[x + y * BADGE_EINK_WIDTH] = (x^y) & 2 ? 0xFF : 0;
			}
		}

		const int gb_offset = (BADGE_EINK_WIDTH - GB_WIDTH)/2;
		const uint8_t * center_image = fb_ram + BADGE_EINK_WIDTH - gb_offset;
		eink_update(
			center_image,
			DISPLAY_FLAG_8BITPIXEL | DISPLAY_FLAG_LUT(2),
			gb_offset,
			gb_offset + GB_WIDTH-1
		);

		done_drawing = 1;

		if (++count == 16)
		{
			unsigned now = sys_micros();
			unsigned delta = now - start;
			ets_printf("%d frames %u us => %d.%02d fps\n",
				count,
				delta,
				((count * 1000000 / delta)),
				((count * 10000) / delta) % 100
			);
			count = 0;
			start = now;
		}
				

	}
}


void vid_init()
{
	fb_ram = calloc(BADGE_EINK_WIDTH*GB_HEIGHT, 1);
	if (!fb_ram)
		die("fb alloc failed\n");

	// use the pitch of the eink display, but the width of
	// the game boy display.
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
	badge_eink_display(NULL, DISPLAY_FLAG_FULL_UPDATE);

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
	framenum++;
	//ets_printf("frame %d enabled=%d\n", framenum, fb.enabled);

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

