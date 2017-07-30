#include "badge.h"

void setup()
{
	Serial.begin(115200);
	Serial.setDebugOutput(true);
	Serial.println("Badge MPR121 Test");

	badge_init();
}


void loop()
{
}
