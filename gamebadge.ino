#include "badge/badge.h"
#include "badge/badge_input.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_fb.h"
#include "badge/badge_leds.h"

#include "loader.h"
#include "mem.h"
#include "rtc.h"
#include "emu.h"
#include "sys.h"


extern "C" {
extern const unsigned char game_rom[];
extern const unsigned int game_rom_len;
extern void burnin();
};

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println("Badge MPR121 Test");

	int rc;

	badge_init();
	badge_leds_enable();

	// map the e-ink to the video
	vid_init();

	if(0) burnin();


	// setup the ROM image
	Serial.println("loading rom");
	rom_load(game_rom, game_rom_len);
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
	uint32_t now = sys_micros();
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
	int start = sys_micros();

	while(sys_micros() - start < us)
	{
		doevents();
		delayMicroseconds(100);
	}
}

unsigned long sys_micros()
{
	//return micros() / 50; // wtf?
	return micros();
}
