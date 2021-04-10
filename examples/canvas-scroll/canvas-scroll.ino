#include "RA8876.h"

#define RA8876_CS        12
#define RA8876_RESET     11
#define RA8876_BACKLIGHT 10

RA8876 tft = RA8876(RA8876_CS, RA8876_RESET);

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

void horizontalScrollTest(int width)
{
  for (int x = 0; x + tft.getWidth() < width; x++)
  {
    tft.setDisplayOffset(x, 0);
    delay(10);
  }
}

void verticalScrollTest(int height)
{
  for (int y = 0; y + tft.getHeight() < height; y++)
  {
    tft.setDisplayOffset(0, y);
    delay(10);
  }  
}

void linearBounceScrollTest(int width, int height)
{
  int x = 0, y = 0;
  int xv = 1, yv = 1;  // Velocities
  
  for (int i = 0; i < (width * 3); i++)
  {
    x += xv;
    y += yv;

    // X axis bounce
    if (x < 0)
    {
      x = 0;
      xv = 1;  
    }
    else if (x + tft.getWidth() > width)
    {
      x = width - tft.getWidth();
      xv = -1;
    }

    // Y axis bounce
    if (y < 0)
    {
      y = 0;
      yv = 1;
    }
    else if (y + tft.getHeight() > height)
    {
      y = height - tft.getHeight();
      yv = -1;
    }

    tft.setDisplayOffset(x, y);
    delay(5);
  }

}

void sineBounceScrollTest(int width, int height)
{
  int x = 0, y = 0;
  int xv = 1;  
  
  for (int i = 0; i < (width * 3); i++)
  {
    y = ((sin(i / 500.0) + 1) / 2.0) * (height - tft.getHeight());
    x = ((sin(i / 250.0) + 1) / 2.0) * (width - tft.getWidth());
    //Serial.print("x: "); Serial.print(x); Serial.print(" y: "); Serial.println(y);

    tft.setDisplayOffset(x, y);
    delay(5);
  }
}

void loop()
{
  Serial.println("Starting test.");

  tft.selectInternalFont(RA8876_FONT_SIZE_32, RA8876_FONT_ENCODING_8859_1);
  tft.setTextScale(2);

  // We want an area twice the width and height of the display
  int width = tft.getWidth() * 2;
  int height = tft.getHeight() * 2;

  // Establish canvas and display regions with proper width 
  tft.setCanvasRegion(0, width);
  tft.setDisplayRegion(0, width);

  // Set the drawing window (within the canvas) to allow drawing to the entire surface
  tft.setCanvasWindow(0, 0, width, height);

  // Clear the memory area
  tft.fillRect(0, 0, width, height, 0);

  // Fill the canvas with some shapes
  int shapeSize = height / 3;
  for (int x = 0; x < width; x += shapeSize)
  {
    tft.fillTriangle(x, 0, x, shapeSize, x + shapeSize, shapeSize, RGB565(0, 255, 0));  // Green triangles
    
    tft.fillCircle(x + (shapeSize / 2), shapeSize + (shapeSize / 2), shapeSize / 2, RGB565(255, 0, 0));  // Red circles
    
    tft.fillRect(x, shapeSize * 2, (x + shapeSize / 2), shapeSize * 2 + (shapeSize / 2), RGB565(0, 0, 255));  // Blue checkerboard
    tft.fillRect(x + (shapeSize / 2), shapeSize * 2 + (shapeSize / 2), x + shapeSize, shapeSize * 3, RGB565(0, 0, 255));
  }

  // Write some text near the middle of the display
  tft.setCursor(0, (height / 2) - 64);
  tft.println("This is the canvas scroll test");
  tft.print("Area is "); tft.print(width); tft.print("x"); tft.println(height);

  delay(1000);

  horizontalScrollTest(width);
  delay(1000);

  verticalScrollTest(height);
  delay(1000);

  linearBounceScrollTest(width, height);
  delay(1000);

  sineBounceScrollTest(width, height);
  delay(5000);

}
