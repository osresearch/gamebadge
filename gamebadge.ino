#include "badge.h"

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
}
