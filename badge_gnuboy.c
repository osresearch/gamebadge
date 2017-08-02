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
	static unsigned long timer;
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
#define GB_OFFSET ((BADGE_EINK_WIDTH - GB_WIDTH)/2)


static uint8_t * fb_ram;
static uint32_t * fb_mono;
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

	// old image not known; do full update
	// black it every few frames
	static unsigned refresh_count;
	int refresh = 0;
	if ((refresh_count++ & 0xF) == 0)
	{
		refresh = 1;
		//badge_eink_display(img, DISPLAY_FLAG_8BITPIXEL | DISPLAY_FLAG_FULL_UPDATE);
		//return;
	}

	lut_flags |= LUT_FLAG_PARTIAL;

// flash to white, looks prety good for a clean flush
static const struct badge_eink_lut_entry lut_flush[] = {
        { .length = 23, .voltages = 0x02, },
        { .length =  4, .voltages = 0x01, },
        { .length = 14, .voltages = 0x99, },
        { .length =  1, .voltages = 0x88, },
        { .length = 0 }
};

// looks pretty good, with moderate ghosting
static const struct badge_eink_lut_entry lut_high_fps[] = {
#if 1
        { .length = 20, .voltages = 0x99, },
#else
        { .length = 2, .voltages = 0x02, },
        { .length = 10, .voltages = 0x99, },
#endif
        { .length = 0 }
};

// 0x40 - 0x60 goes to full black
static const struct badge_eink_lut_entry lut_test[] = {
        //{ .length = 10, .voltages = 0x99, },
        { .length = 4, .voltages = 0x01, },
        { .length = 4, .voltages = 0x80, },
        { .length = 0 }
};


	struct badge_eink_update eink_upd = {
		//.lut       = lut_mode > 0 ? lut_mode - 1 : BADGE_EINK_LUT_DEFAULT,
		//.lut_flags = lut_flags,
		.lut = BADGE_EINK_LUT_CUSTOM,
		.lut_custom = refresh ? lut_flush : lut_high_fps,
		.reg_0x3a  = 26,   // 26 dummy lines per gate
		.reg_0x3b  = 0x08, // 62us per line
		.y_start   = refresh ? 0 : y_start,
		.y_end     = refresh ? BADGE_EINK_WIDTH-1 : y_end,
	};

	badge_eink_update(img, &eink_upd);
#else
	// black it every few frames
	badge_eink_display_greyscale(img, flags, 4);
#endif
}

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

		eink_update(
			fb_mono,
			DISPLAY_FLAG_LUT(1),
			GB_OFFSET,
			GB_OFFSET + GB_WIDTH-1
		);

		done_drawing = 1;
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
	static unsigned long last_draw;
	static unsigned skipped;

	if(!fb.enabled)
	{
		if (done_drawing)
			fb.enabled = 1;
		skipped++;
		return;
	}

	static int framenum;
	framenum++;
	unsigned long delta = sys_micros() - last_draw;
	ets_printf("frame %d: skip %d %lu\n", framenum, skipped, delta);
	skipped = 0;

/*
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
*/

/*
	for(int i = 0 ; i < 128 ; i++)
	{
		ets_printf("%02x", fb_ram[i + i * BADGE_EINK_WIDTH]);
		if (i % 32 == 31)
			ets_printf("\n");
	}
*/

	// post process the image
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
			//fb_ram[x + y * BADGE_EINK_WIDTH] = (x^y) & 2 ? 0xFF : 0;
			fb_ram[x + y * BADGE_EINK_WIDTH] = 0xFF;
		}
	}

	// translate it to a monochrome bitmap, centered on the screen
	const uint8_t * center_image = fb_ram + BADGE_EINK_WIDTH - GB_OFFSET;
	fb_mono = badge_eink_tmpbuf;
	badge_eink_create_bitplane(center_image, fb_mono, 0x80, DISPLAY_FLAG_8BITPIXEL);

	// mark that the fb is in use and wake the drawing task
	done_drawing = fb.enabled = 0;
	xSemaphoreGive(fb_mutex);
	last_draw = sys_micros();
}
