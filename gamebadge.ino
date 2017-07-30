#include "badge.h"
#include "badge_input.h"


void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println("Badge MPR121 Test");

	int rc = badge_init();
	if (rc != ESP_OK)
	{
		while(1)
		{
			Serial.print(rc);
			Serial.println(" failed");
			delay(1000);
		}
	}
}


void loop()
{
	uint32_t button = badge_input_get_event(1000);
	Serial.print(button);
	Serial.print(" ");
	Serial.println(badge_input_button_state, HEX);
}
