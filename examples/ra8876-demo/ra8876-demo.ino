#include "RA8876.h"

#define RA8876_CS        12
#define RA8876_RESET     11
#define RA8876_BACKLIGHT 10


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

  tft.clearScreen(0);
  
  tft.colorBarTest(true);
  delay(1000);
  tft.colorBarTest(false);

  pixelTest();

  delay(1000);

  triangleTest();

  delay(1000);
  
  circleTest();

  delay(1000);
  
  gradientTest();

  delay(1000);

  textTest();

  delay(1000);
}

void gradientTest()
{
  Serial.println("Gradient test.");

  tft.clearScreen(0);

  int width = tft.getWidth();
  int barHeight = tft.getHeight() / 4;

  uint32_t starttime = millis();
  
  for (int i = 0; i <= 255; i++)
  {
    tft.fillRect((width / 256.0) * i, 0, (width / 256.0) * (i + 1) - 1, barHeight, RGB565(i, 0, 0));
    tft.fillRect((width / 256.0) * i, barHeight, (width / 256.0) * (i + 1) - 1, barHeight * 2, RGB565(0, i, 0));
    tft.fillRect((width / 256.0) * i, barHeight * 2, (width / 256.0) * (i + 1) - 1, barHeight * 3, RGB565(0, 0, i));
    tft.fillRect((width / 256.0) * i, barHeight * 3, (width / 256.0) * (i + 1) - 1, barHeight * 4, RGB565(i, i, i));
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Gradient test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void pixelTest()
{
  Serial.println("Pixel test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  uint16_t colors[] = {RGB565(255, 0, 0), RGB565(0, 255, 0), RGB565(0, 0, 255)};

  uint32_t starttime = millis();

  for (int c = 0; c < 3; c++)
  {
    for (int i = 0; i < 3000; i++)
    {
      int x = random(0, width);
      int y = random(0, height);
      
      tft.drawPixel(x, y, colors[c]);
    }
  }
    
  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Pixel test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void triangleTest()
{
  Serial.println("Triangle test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  uint32_t starttime = millis();

  for (int i = 0; i < 2000; i++)
  {
    int x1 = random(0, width);
    int y1 = random(0, height);
    int x2 = random(0, width);
    int y2 = random(0, height);
    int x3 = random(0, width);
    int y3 = random(0, height);

    uint16_t color = RGB565(random(0, 255), random(0, 255), random(0, 255));

    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Triangle test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void circleTest()
{
  Serial.println("Circle test.");

  int width = tft.getWidth();
  int height = tft.getHeight();

  uint32_t starttime = millis();

  for (int i = 0; i < 2000; i++)
  {
    int x = random(0, width);
    int y = random(0, height);
    int r = random(0, 384);

    uint16_t color = RGB565(random(0, 255), random(0, 255), random(0, 255));

    tft.fillCircle(x, y, r, color);
  }

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Circle test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void textTest()
{
  Serial.println("Text test.");

  uint32_t starttime = millis();

  for (int s = 1; s <= 4; s++)
  {
    tft.setTextScale(s);
    
    for (int i = 0; i < 3; i++)
    {
      tft.setCursor((tft.getWidth() / 3) * i, tft.getCursorY());
      tft.selectInternalFont((enum FontSize) i);
      tft.print("Hello");
    }

    tft.println();
  }

  tft.setCursor(0, 32 * 10);
  tft.setTextScale(1);

  tft.selectInternalFont(RA8876_FONT_SIZE_16X32, RA8876_FONT_ENCODING_8859_1);
  tft.println("Latin 1: na\xEFve");  // naïve

  tft.selectInternalFont(RA8876_FONT_SIZE_16X32, RA8876_FONT_ENCODING_8859_2);
  tft.println("Latin 2: \xE8" "a\xE8kalica");

  tft.selectInternalFont(RA8876_FONT_SIZE_16X32, RA8876_FONT_ENCODING_8859_4);
  tft.println("Latin 4: gie\xF0" "at");  // gieđat

  tft.selectInternalFont(RA8876_FONT_SIZE_16X32, RA8876_FONT_ENCODING_8859_5);
  tft.println("Latin 5: \xD2\xD5\xD4\xD8");  // веди

  tft.selectInternalFont(RA8876_FONT_SIZE_16X32);
  tft.print("Symbols: ");
  tft.putChars((const uint8_t *) "\x00\x01\x02\x03\x04\x05\x06\x07", 8);
  tft.putChars((const uint8_t *) "\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 8);

  uint32_t elapsedtime = millis() - starttime;
  Serial.print("Text test took "); Serial.print(elapsedtime); Serial.println(" ms");
}

void loop()
{
  

}
