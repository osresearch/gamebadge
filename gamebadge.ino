#include "badge/badge.h"
#include "badge/badge_input.h"
#include "badge/badge_eink.h"
#include "badge/badge_eink_fb.h"

#include "loader.h"
#include "mem.h"
#include "rtc.h"
#include "emu.h"


extern "C" const unsigned char rom_image[];

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println("Badge MPR121 Test");

	int rc;

	rc = badge_init();
	if (rc != ESP_OK)
	{
		while(1)
		{
			Serial.print(rc);
			Serial.println(" failed");
			delay(1000);
		}
	}

	rc = badge_eink_fb_init();
	if (rc != ESP_OK)
	{
		while(1)
		{
			Serial.print(rc);
			Serial.println(" failed 3");
			delay(1000);
		}
	}

	// turn the display all black
	memset(badge_eink_fb, 0x00, BADGE_EINK_FB_LEN);
	badge_eink_display(badge_eink_fb, DISPLAY_FLAG_LUT(0));

	// startup the emulator
	emu_reset();

	// setup the ROM image
	Serial.println("loading rom");
	rom_load(rom_image, 65536);
	Serial.println("loaded!");
	mem_updatemap();

}


void loop()
{
	Serial.println("starting emulator");
	emu_run();

	uint32_t button = badge_input_get_event(10);
	if (button != 0)
	{
		Serial.print(button);
		Serial.print(" ");
		Serial.println(badge_input_button_state, HEX);

	for(int x = 0 ; x < BADGE_EINK_WIDTH ; x++)
	{
		for(int y = 0 ; y < BADGE_EINK_HEIGHT ; y++)
		{
			badge_eink_fb[x + y * BADGE_EINK_WIDTH] = x + y + button;
		}
	}

	badge_eink_display(badge_eink_fb, DISPLAY_FLAG_GREYSCALE | DISPLAY_FLAG_LUT(2));
	}
}


extern "C" void serial_int(int x);

void serial_int(int x)
{
	Serial.println(x);
}


extern "C" int sys_elapsed(uint32_t * prev);

int sys_elapsed(uint32_t * prev)
{
	uint32_t now = micros();
	uint32_t delta = now - *prev;
	*prev = now;
	return delta;
}

extern "C" void sys_sleep(int us);
void sys_sleep(int us)
{
	delayMicroseconds(us);
}
