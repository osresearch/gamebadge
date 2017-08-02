#include "badge/badge.h"
#include "badge/badge_input.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_fb.h"

#include "loader.h"
#include "mem.h"
#include "rtc.h"
#include "emu.h"
#include "sys.h"


extern "C" {
extern const unsigned char rom_image[];
extern const unsigned int rom_image_size;
};

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println("Badge MPR121 Test");

	int rc;

	badge_init();

	// map the e-ink to the video
	vid_init();

#if 0
	// turn the display all black
	uint8_t * buf = (uint8_t*) malloc(BADGE_EINK_WIDTH*BADGE_EINK_HEIGHT);
	while(1)
	{
	for(int i = 0 ; i < 1 ; i++)
	{
		memset(buf, 0xFF, BADGE_EINK_WIDTH*BADGE_EINK_HEIGHT);
		badge_eink_display_one_layer(buf, DISPLAY_FLAG_FULL_UPDATE);
		for(int j = 0 ; j < 10 ; j++)
			badge_eink_display_one_layer(buf, DISPLAY_FLAG_LUT(2));
		delay(10000);
		memset(buf, 0, BADGE_EINK_WIDTH*BADGE_EINK_HEIGHT);
		badge_eink_display_one_layer(buf, DISPLAY_FLAG_FULL_UPDATE);
		for(int j = 0 ; j < 10 ; j++)
			badge_eink_display_one_layer(buf, DISPLAY_FLAG_LUT(2));
		delay(10000);
	}
	
	for(int i=0 ; i < 10 ; i++)
	{
		for(int x = 0 ; x < BADGE_EINK_WIDTH ; x++)
			for(int y = 0 ; y < BADGE_EINK_HEIGHT ; y++)
				buf[x + y * BADGE_EINK_WIDTH] = ((int)random(2)) ? 0xFF : 0;
					
		badge_eink_display_one_layer(buf, DISPLAY_FLAG_FULL_UPDATE);
		for(int j = 0 ; j < 10 ; j++)
			badge_eink_display_one_layer(buf, DISPLAY_FLAG_LUT(2));
	}
	}
#endif

	// setup the ROM image
	Serial.println("loading rom");
	rom_load(rom_image, rom_image_size);
	Serial.println("loaded!");

	// startup the emulator
	emu_reset();

	Serial.println("starting emulator");
	emu_run();
}


void loop()
{
	die("ended up in loop?");
}


extern "C" void serial_int(int x);

void serial_int(int x)
{
	Serial.println(x);
}


int sys_elapsed(uint32_t * prev)
{
	uint32_t now = micros();
	uint32_t delta = now - *prev;
	*prev = now;
//ets_printf("elapsed: %d\n", delta);
	return delta;
}

void sys_sleep(int us)
{
	if (us < 0)
		return;

//ets_printf("usleep(%d)\n", us);
	int start = micros();

	while(micros() - start < us)
	{
		doevents();
		delayMicroseconds(100);
	}
}

unsigned int sys_micros()
{
	return micros();
}
