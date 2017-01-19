#include "RA8876.h"

#define RA8876_CS        10
#define RA8876_RESET     9
#define RA8876_BACKLIGHT 8


RA8876 tft = RA8876(RA8876_CS, RA8876_RESET);

void setup()
{
  Serial.begin(9600);

  delay(1000);

  while (!Serial);

  Serial.println("Starting up...");

  pinMode(RA8876_BACKLIGHT, OUTPUT);  // Set backlight pin to OUTPUT mode
  digitalWrite(RA8876_BACKLIGHT, HIGH);  // Turn on backlight
  
  if (!tft.init())
  {
    Serial.println("Could not initialize RA8876");
  }

  Serial.println("Startup complete...");

  //tft.colorBarTest(true);

  // Clear screen
  tft.fillRect(0, 0, 1023, 599, 0x0000);

  // Draw diagonal lines
  for (int i = 0; i < 300; i++)
  {
    tft.drawPixel(i, i, RGB565(i % 256, 0, 0));
    tft.drawPixel(i + 5, i, RGB565(0, i % 256, 0));
    tft.drawPixel(i + 10, i, RGB565(0, 0, i % 256));
    tft.drawPixel(i + 15, i, RGB565(i % 256, i % 256, i % 256));
    delay(50);
  }
}

void loop()
{


}
