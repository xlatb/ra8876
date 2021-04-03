#include "RA8876.h"

#define RA8876_CS        12
#define RA8876_RESET     11
#define RA8876_BACKLIGHT 10

RA8876 tft = RA8876(RA8876_CS, RA8876_RESET);

// NOTE: Using these fonts requires that your RA8876 has an optional font chip attached.
// Uncomment the line that matches your font chip:
//ExternalFontRom rom = RA8876_FONT_ROM_GT21L16T1W;  // GT21L16T1W (aka ER3300-1)
//ExternalFontRom rom = RA8876_FONT_ROM_GT30L16U2W;  // GT30L16U2W (aka ER3301-1)
ExternalFontRom rom = RA8876_FONT_ROM_GT30L24T3Y;  // GT30L24T3Y (aka ER3303-1)
//ExternalFontRom rom = RA8876_FONT_ROM_GT30L32S4W;  // GT30L32S4W (aka ER3304-1)

void setup()
{
  Serial.begin(9600);

  delay(1000);

  while (!Serial && (millis() < 5000));

  Serial.println("Initializing display...");

  pinMode(RA8876_BACKLIGHT, OUTPUT);  // Set backlight pin to OUTPUT mode
  digitalWrite(RA8876_BACKLIGHT, HIGH);  // Turn on backlight

  if (!tft.init())
  {
    Serial.println("Could not initialize RA8876");
  }

  Serial.println("Display init completed.");
}

void externalFontTestGT30L24T3Y()
{
  tft.println("Test for GT30L24T3Y (aka ER3303-1)\n");
  
  tft.initExternalFontRom(0, rom);
  
  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_ASCII);
  tft.println("Fixed 8x16.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_UNICODE, RA8876_FONT_FLAG_XLAT_FULLWIDTH);
  tft.println("Fixed 15x16.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_24, RA8876_FONT_ENCODING_UNICODE, RA8876_FONT_FLAG_XLAT_FULLWIDTH);
  tft.println("Fixed 24x24.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_ARIAL, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_ASCII);
  tft.println("Arial 16.");
  showFont();
  
  tft.selectExternalFont(RA8876_FONT_FAMILY_ARIAL, RA8876_FONT_SIZE_24, RA8876_FONT_ENCODING_ASCII);
  tft.println("Arial 24.");
  showFont();
}

void externalFontTestGT30L32S4W()
{
  tft.println("Test for GT30L32S4W (aka ER3304-1)\n");

  int y = tft.getCursorY();
  
  tft.initExternalFontRom(0, rom);
  
  // --- Fixed
  
  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_ASCII);
  tft.println("Fixed 8x16.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_24, RA8876_FONT_ENCODING_ASCII);
  tft.println("Fixed 12x24.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_FIXED, RA8876_FONT_SIZE_32, RA8876_FONT_ENCODING_ASCII);
  tft.println("Fixed 16x32.");
  showFont();

  delay(2000);
  tft.fillRect(0, y, tft.getWidth(), tft.getHeight(), 0);
  tft.setCursor(0, y);

  // --- Arial

  tft.selectExternalFont(RA8876_FONT_FAMILY_ARIAL, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_ASCII);
  tft.println("Arial 16.");
  showFont();
  
  tft.selectExternalFont(RA8876_FONT_FAMILY_ARIAL, RA8876_FONT_SIZE_24, RA8876_FONT_ENCODING_ASCII);
  tft.println("Arial 24.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_ARIAL, RA8876_FONT_SIZE_32, RA8876_FONT_ENCODING_ASCII);
  tft.println("Arial 32.");
  showFont();

  delay(2000);
  tft.fillRect(0, y, tft.getWidth(), tft.getHeight(), 0);
  tft.setCursor(0, y);

  // --- Times

  tft.selectExternalFont(RA8876_FONT_FAMILY_TIMES, RA8876_FONT_SIZE_16, RA8876_FONT_ENCODING_ASCII);
  tft.println("Times 16.");
  showFont();
  
  tft.selectExternalFont(RA8876_FONT_FAMILY_TIMES, RA8876_FONT_SIZE_24, RA8876_FONT_ENCODING_ASCII);
  tft.println("Times 24.");
  showFont();

  tft.selectExternalFont(RA8876_FONT_FAMILY_TIMES, RA8876_FONT_SIZE_32, RA8876_FONT_ENCODING_ASCII);
  tft.println("Times 32.");
  showFont();
}

void showFont()
{
  tft.println("ABCDEFGHIJKLMNOPQRSTUVWXYZ !@#$%^&*()");
  tft.println("abcdefghijklmnopqrstuvwxyz 0123456789");
  tft.println("_+-={}[];'\\|:\",./<>?`~");
  tft.println();
}

void loop()
{
  Serial.println("Starting test.");

  tft.clearScreen(0);

  tft.selectInternalFont(RA8876_FONT_SIZE_32, RA8876_FONT_ENCODING_8859_1);
  tft.println("External font test\n");

  switch (rom)
  {
    case RA8876_FONT_ROM_GT30L24T3Y:
      externalFontTestGT30L24T3Y();
      break;
    case RA8876_FONT_ROM_GT30L32S4W:
      externalFontTestGT30L32S4W();
      break;
    default:
      tft.println("Sorry, no test yet for that font ROM chip.\n");
      break;
  }
  
  delay(5000);
}
