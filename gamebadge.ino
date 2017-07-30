#include "badge.h"
#include "badge_input.h"
#include "badge_eink.h"
#include "badge_eink_fb.h"

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
}


void loop()
{
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
