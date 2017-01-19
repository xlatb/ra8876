#pragma GCC diagnostic warning "-Wall"
#include "RA8876.h"

SdramInfo defaultSdramInfo =
{
  120, // 120 MHz
  3,   // CAS latency 3
  4,   // 4 banks
  12,  // 12-bit row addresses
  9,   // 9-bit column addresses
  64   // 64 millisecond refresh time
};

DisplayInfo defaultDisplayInfo =
{
  1024,   // Display width
  600,    // Display height
  50000,  // Pixel clock in kHz
  
  160,    // Horizontal front porch
  160,    // Horizontal back porch
  70,     // HSYNC pulse width

  12,     // Vertical front porch
  23,     // Vertical back porch
  10      // VSYNC pulse width
};

void RA8876::writeCmd(uint8_t x)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8876_CMD_WRITE);
  SPI.transfer(x);
  digitalWrite(m_csPin, HIGH);
}

void RA8876::writeData(uint8_t x)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8876_DATA_WRITE);
  SPI.transfer(x);
  digitalWrite(m_csPin, HIGH);
}

uint8_t RA8876::readData(void)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8876_DATA_READ);
  uint8_t x = SPI.transfer(0);
  digitalWrite(m_csPin, HIGH);
  return x;
}

// Reads the special status register.
// This register uses a special cycle type instead of having an address like other registers.
// See data sheet section 19.1.
uint8_t RA8876::readStatus(void)
{
  digitalWrite(m_csPin, LOW);
  SPI.transfer(RA8876_STATUS_READ);
  uint8_t x = SPI.transfer(0);
  digitalWrite(m_csPin, HIGH);
  return x;
}

void RA8876::writeReg(uint8_t reg, uint8_t x)
{
  writeCmd(reg);
  writeData(x);
}

uint8_t RA8876::readReg(uint8_t reg) 
{
  writeCmd(reg);
  return readData();
}

RA8876::RA8876(int csPin, int resetPin)
{
  m_csPin    = csPin;
  m_resetPin = resetPin;

  m_width  = 0;
  m_height = 0;
  m_depth  = 0;

  m_oscClock = 10000;  // 10000kHz or 10MHz

  m_sdramInfo = &defaultSdramInfo;

  m_displayInfo = &defaultDisplayInfo;
}

// Trigger a hardware reset.
void RA8876::hardReset(void)
{
  delay(5);
  digitalWrite(m_resetPin, LOW);
  delay(5);
  digitalWrite(m_resetPin, HIGH);
  delay(5);

  return;
}

// Trigger a soft reset. Note that the data sheet section 19.2 says that this only resets the
//  "internal state machine", not any configuration registers.
void RA8876::softReset(void)
{
  SPI.beginTransaction(m_spiSettings);

  // Trigger soft reset
  writeReg(RA8876_REG_SRR, 0x01);
  delay(5);

  // Wait for status register to show "normal operation".
  uint8_t status;
  for (int i = 0; i < 250; i++)
  {
    delay(1);
    
    if (((status = readStatus()) & 0x02) == 0)
      break;
  }

  SPI.endTransaction();

  return;
}

// Given a target frequency in kHz, finds PLL parameters k and n to reach as
//  close as possible to the target frequency without exceeding it.
// The k parameter will be constrained to the range 1..kMax.
// Returns true iff PLL params were found, even if not an exact match.
bool RA8876::calcPllParams(uint32_t targetFreq, int kMax, PllParams *pll)
{
  bool found = false;
  int foundk, foundn;
  uint32_t foundFreq;
  uint32_t foundError;  // Amount lower than requested frequency
  
  // k of 0 (i.e. 2 ** 0 = 1) is possible, but not sure if it's a good idea.
  for (int testk = 1; testk <= kMax; testk++)
  {
    if (m_oscClock % (1 << testk))
      continue;  // Step size with this k would be fractional

    int testn = (targetFreq / (m_oscClock / (1 << testk))) - 1;
    if ((testn < 1) || (testn > 63))
      continue;  // Param n out of range for this k
      
    // Fvco constraint found in data sheet section 6.1.2
    uint32_t fvco = m_oscClock * (testn + 1);
    if ((fvco < 100000) && (fvco > 600000))
      continue;  // Fvco out of range
      
    // Found some usable params, but perhaps at a lower frequency than requested.
    uint32_t freq = (m_oscClock * (testn + 1)) / (1 << testk);
    uint32_t error = targetFreq - freq;
    if ((!found) || (found && (foundError > error)))
    {
      found = true;
      foundk = testk;
      foundn = testn;
      foundFreq = freq;
      foundError = error;

      // No need to keep searching if the frequency match was exact
      if (foundError == 0)
        break;
    }
  }

  if (found)
  {
    pll->freq = foundFreq;
    pll->k    = foundk;
    pll->n    = foundn;
  }

  return found;
}

// Calculates the clock frequencies and their PLL parameters.
bool RA8876::calcClocks(void)
{
  // Data sheet section 5.2 gives max clocks:
  //  memClock : 166 MHz
  //  coreClock: 120 MHz (133MHz if not using internal font)
  //  scanClock: 100 MHz

  // Mem clock target is the same as SDRAM speed, but capped at 166 MHz
  uint32_t memClock = m_sdramInfo->speed * 1000;
  if (memClock > 166000)
    memClock = 166000;

  if (!calcPllParams(memClock, 3, &m_memPll))
    return false;

  // Core clock target will be the same as the mem clock, but capped to
  //  120 MHz, because that is the max frequency if we want to use the
  //  internal font.
  uint32_t coreClock = m_memPll.freq;
  if (coreClock > 120000)
    coreClock = 120000;

  if (!calcPllParams(coreClock, 3, &m_corePll))
    return false;

  // Scan clock target will be the display's dot clock, but capped at 100 MHz
  uint32_t scanClock = m_displayInfo->dotClock;
  if (scanClock > 100000)
    scanClock = 100000;

  if (!calcPllParams(scanClock, 7, &m_scanPll))
    return false;

  dumpClocks();

  // Data sheet section 6.1.1 rules:
  // 1. Core clock must be less than or equal to mem clock
  if (m_corePll.freq > m_memPll.freq)
    return false;

  // 2. Core clock must be greater than half mem clock
  if ((m_corePll.freq * 2) <= m_memPll.freq)
    return false;

  // 3. Core clock must be greater than (scan clock * 1.5)
  if (m_corePll.freq <= (m_scanPll.freq + (m_scanPll.freq >> 1)))
    return false;
      
  return true;
}

// Dump clock info to serial monitor.
void RA8876::dumpClocks(void)
{
  Serial.println("\nMem\n---");
  Serial.print("Requested kHz: "); Serial.println(m_sdramInfo->speed * 1000);
  Serial.print("Actual kHz   : "); Serial.println(m_memPll.freq);
  Serial.print("PLL k        : "); Serial.println(m_memPll.k);
  Serial.print("PLL n        : "); Serial.println(m_memPll.n);

  Serial.println("\nCore\n----");
  Serial.print("kHz          : "); Serial.println(m_corePll.freq);
  Serial.print("PLL k        : "); Serial.println(m_corePll.k);
  Serial.print("PLL n        : "); Serial.println(m_corePll.n);
  
  Serial.println("\nScan\n----");
  Serial.print("Requested kHz: "); Serial.println(m_displayInfo->dotClock);
  Serial.print("Actual kHz   : "); Serial.println(m_scanPll.freq);
  Serial.print("PLL k        : "); Serial.println(m_scanPll.k);
  Serial.print("PLL n        : "); Serial.println(m_scanPll.n);

  // TODO: Frame rate?

  return;
}

bool RA8876::initPLL(void)
{
  Serial.println("init PLL");

  SPI.beginTransaction(m_spiSettings);

  //Serial.print("DRAM_FREQ "); Serial.println(m_memPll.freq);
  //Serial.print("7: "); Serial.println(m_memPll.k << 1);
  //Serial.print("8: "); Serial.println(m_memPll.n);
  writeReg(RA8876_REG_MPLLC1, m_memPll.k << 1);
  writeReg(RA8876_REG_MPLLC2, m_memPll.n);

  //Serial.print("CORE_FREQ "); Serial.println(m_corePll.freq);
  //Serial.print("9: "); Serial.println(m_corePll.k << 1);
  //Serial.print("A: "); Serial.println(m_corePll.n);
  writeReg(RA8876_REG_SPLLC1, m_corePll.k << 1);
  writeReg(RA8876_REG_SPLLC2, m_corePll.n);

  // Per the data sheet, there are two divider fields for the scan clock, but the math seems
  //  to work out if we treat k as a single 3-bit number in bits 3..1.
  //Serial.print("SCAN_FREQ "); Serial.println(m_scanPll.freq);
  //Serial.print("5: "); Serial.println(m_scanPll.k << 1);
  //Serial.print("6: "); Serial.println(m_scanPll.n);
  writeReg(RA8876_REG_PPLLC1, m_scanPll.k << 1);
  writeReg(RA8876_REG_PPLLC2, m_scanPll.n);

  // Toggle bit 7 of the CCR register to trigger a reconfiguration of the PLLs
  writeReg(RA8876_REG_CCR, 0x00);
  delay(2);
  writeReg(RA8876_REG_CCR, 0x80);
  delay(2);

  uint8_t ccr = readReg(RA8876_REG_CCR);

  SPI.endTransaction();

  return (ccr & 0x80) ? true : false;
}

// Initialize SDRAM interface.
bool RA8876::initMemory(SdramInfo *info)
{ 
  Serial.println("init memory");    

  uint32_t sdramRefreshRate;
  uint8_t sdrar = 0x00;
  uint8_t sdrmd = 0x00;

  // Refresh rate
  sdramRefreshRate = ((uint32_t) info->refresh * info->speed * 1000) >> info->rowBits;

  // Number of banks
  if (info->banks == 2)
    ;  // NOP
  else if (info->banks == 4)
    sdrar |= 0x20;
  else
    return false;  // Unsupported number of banks

  // Number of row bits
  if ((info->rowBits < 11) || (info->rowBits > 13))
    return false;  // Unsupported number of row bits
  else 
    sdrar |= ((info->rowBits - 11) & 0x03) << 3;

  // Number of column bits
  if ((info->colBits < 8) || (info->colBits > 12))
    return false;  // Unsupported number of column bits
  else
    sdrar |= info->colBits & 0x03;

  // CAS latency
  if ((info->casLatency < 2) || (info->casLatency > 3))
    return false;  // Unsupported CAS latency
  else
    sdrmd |= info->casLatency & 0x03;

  SPI.beginTransaction(m_spiSettings);

  Serial.print("SDRAR: "); Serial.println(sdrar);  // Expected: 0x29 (41 decimal)
  writeReg(RA8876_REG_SDRAR, sdrar);

  Serial.print("SDRMD: "); Serial.println(sdrmd);
  writeReg(RA8876_REG_SDRMD, sdrmd);
  
  Serial.print("sdramRefreshRate: "); Serial.println(sdramRefreshRate);
  writeReg(RA8876_REG_SDR_REF_ITVL0, sdramRefreshRate & 0xFF);
  writeReg(RA8876_REG_SDR_REF_ITVL1, sdramRefreshRate >> 8);

  // Trigger SDRAM initialization
  writeReg(RA8876_REG_SDRCR, 0x01);

  // Wait for SDRAM to be ready
  uint8_t status;
  for (int i = 0; i < 250; i++)
  {
    delay(1);
    
    if ((status = readStatus()) & 0x40)
      break;
  }

  SPI.endTransaction();

  Serial.println(status);
  
  return (status & 0x40) ? true : false;
}

bool RA8876::initDisplay()
{
  SPI.beginTransaction(m_spiSettings);
  
  // Set chip config register
  uint8_t ccr = readReg(RA8876_REG_CCR);
  ccr &= 0xE7;  // 24-bit LCD output
  ccr &= 0xFE;  // 8-bit host data bus
  writeReg(RA8876_REG_CCR, ccr);

  writeReg(RA8876_REG_MACR, 0x00);  // Direct write, left-to-right-top-to-bottom memory

  writeReg(RA8876_REG_ICR, 0x00);  // Graphics mode, memory is SDRAM

  uint8_t dpcr = readReg(RA8876_REG_DPCR);
  dpcr &= 0xFB;  // Vertical scan top to bottom
  dpcr &= 0xF8;  // Colour order RGB
  dpcr |= 0x80;  // Panel fetches PDAT at PCLK falling edge
  writeReg(RA8876_REG_DPCR, dpcr);

  uint8_t pcsr = readReg(RA8876_REG_PCSR);
  pcsr |= 0x80;  // XHSYNC polarity high
  pcsr |= 0x40;  // XVSYNC polarity high
  pcsr &= 0xDF;  // XDE polarity high
  writeReg(RA8876_REG_PCSR, pcsr);

  // Set display width
  writeReg(RA8876_REG_HDWR, (m_displayInfo->width / 8) - 1);
  writeReg(RA8876_REG_HDWFTR, (m_displayInfo->width % 8));

  // Set display height
  writeReg(RA8876_REG_VDHR0, (m_displayInfo->height - 1) & 0xFF);
  writeReg(RA8876_REG_VDHR1, (m_displayInfo->height - 1) >> 8);

  // Set horizontal non-display (back porch)
  writeReg(RA8876_REG_HNDR, (m_displayInfo->hBackPorch / 8) - 1);
  writeReg(RA8876_REG_HNDFTR, (m_displayInfo->hBackPorch % 8));

  // Set horizontal start position (front porch)
  writeReg(RA8876_REG_HSTR, ((m_displayInfo->hFrontPorch + 4) / 8) - 1);

  // Set HSYNC pulse width
  writeReg(RA8876_REG_HPWR, ((m_displayInfo->hPulseWidth + 4) / 8) - 1);

  // Set vertical non-display (back porch)
  writeReg(RA8876_REG_VNDR0, (m_displayInfo->vBackPorch - 1) & 0xFF);
  writeReg(RA8876_REG_VNDR1, (m_displayInfo->vBackPorch - 1) >> 8);

  // Set vertical start position (front porch)
  writeReg(RA8876_REG_VSTR, m_displayInfo->vFrontPorch - 1);

  // Set VSYNC pulse width
  writeReg(RA8876_REG_VPWR, m_displayInfo->vPulseWidth - 1);

  // Set main window to 16 bits per pixel
  writeReg(RA8876_REG_MPWCTR, 0x04);  // PIP windows disabled, 16-bpp, enable sync signals

  // Set main window start address to 0
  writeReg(RA8876_REG_MISA0, 0);
  writeReg(RA8876_REG_MISA1, 0);
  writeReg(RA8876_REG_MISA2, 0);
  writeReg(RA8876_REG_MISA3, 0);

  // Set main window image width
  writeReg(RA8876_REG_MIW0, m_width & 0xFF);
  writeReg(RA8876_REG_MIW1, m_width >> 8);

  // Set main window start coordinates
  writeReg(RA8876_REG_MWULX0, 0);
  writeReg(RA8876_REG_MWULX1, 0);
  writeReg(RA8876_REG_MWULY0, 0);
  writeReg(RA8876_REG_MWULY1, 0);

  // Set canvas start address
  writeReg(RA8876_REG_CVSSA0, 0);
  writeReg(RA8876_REG_CVSSA1, 0);
  writeReg(RA8876_REG_CVSSA2, 0);
  writeReg(RA8876_REG_CVSSA3, 0);

  // Set canvas width
  writeReg(RA8876_REG_CVS_IMWTH0, m_width & 0xFF);
  writeReg(RA8876_REG_CVS_IMWTH1, m_width >> 8);

  // Set active window start coordinates
  writeReg(RA8876_REG_AWUL_X0, 0);
  writeReg(RA8876_REG_AWUL_X1, 0);
  writeReg(RA8876_REG_AWUL_Y0, 0);
  writeReg(RA8876_REG_AWUL_Y1, 0);

  // Set active window dimensions
  writeReg(RA8876_REG_AW_WTH0, m_width & 0xFF);
  writeReg(RA8876_REG_AW_WTH1, m_width >> 8);
  writeReg(RA8876_REG_AW_HT0, m_height & 0xFF);
  writeReg(RA8876_REG_AW_HT1, m_height >> 8);

  // Set canvas addressing mode/colour depth
  uint8_t aw_color = 0x00;  // 2d addressing mode
  if (m_depth == 16)
    aw_color |= 0x01;
  else if (m_depth == 24)
    aw_color |= 0x02;
  writeReg(RA8876_REG_AW_COLOR, aw_color);
  
  // Turn on display
  dpcr = readReg(RA8876_REG_DPCR);
  dpcr |= 0x40;  // Display on
  writeReg(RA8876_REG_DPCR, dpcr);

  // TODO: Track backlight pin and turn on backlight

  SPI.endTransaction();

  return true;
}

bool RA8876::init(void)
{
  m_width  = m_displayInfo->width;
  m_height = m_displayInfo->height;
  m_depth  = 16;

  // Set up chip select pin
  pinMode(m_csPin, OUTPUT);
  digitalWrite(m_csPin, HIGH);

  // Set up reset pin, if provided
  if (m_resetPin >= 0)
  {
    pinMode(m_resetPin, OUTPUT);
    digitalWrite(m_resetPin, HIGH);

    hardReset();
  }

  if (!calcClocks())
  {
    Serial.println("calcClocks failed");
    return false;
  }

  SPI.begin();

  m_spiSettings = SPISettings(RA8876_SPI_SPEED, MSBFIRST, SPI_MODE3);

  // SPI is now up, so we can do a soft reset if no hard reset was possible earlier
  if (m_resetPin < 0)
    softReset();

  if (!initPLL())
  {
    Serial.println("initPLL failed");
    return false;
  }

  if (!initMemory(m_sdramInfo))
  {
    Serial.println("initMemory failed");
    return false;
  }

  if (!initDisplay())
  {
    Serial.println("initDisplay failed");
    return false;
  }

  return true;
}

// Show colour bars of 8 colours in repeating horizontal bars.
// This does not alter video memory, but rather instructs the video controller to display
//  the pattern rather than the contents of memory.
void RA8876::colorBarTest(bool enabled)
{
  SPI.beginTransaction(m_spiSettings);

  uint8_t dpcr = readReg(RA8876_REG_DPCR);

  if (enabled)
    dpcr = dpcr | 0x20;
  else
    dpcr = dpcr & ~0x20;

  writeReg(RA8876_REG_DPCR, dpcr);

  SPI.endTransaction();
}

void RA8876::drawPixel(int x, int y, uint16_t color)
{
  //Serial.println("drawPixel");
  //Serial.println(readStatus());
  
  SPI.beginTransaction(m_spiSettings);

  writeReg(RA8876_REG_CURH0, x & 0xFF);
  writeReg(RA8876_REG_CURH1, x >> 8);

  writeReg(RA8876_REG_CURV0, y & 0xFF);
  writeReg(RA8876_REG_CURV1, y >> 8);

  writeReg(RA8876_REG_MRWDP, color & 0xFF);
  writeReg(RA8876_REG_MRWDP, color >> 8);
  
  SPI.endTransaction();
}

void RA8876::fillRect(int x1, int y1, int x2, int y2, uint16_t color)
{
  Serial.println("fillRect");

  SPI.beginTransaction(m_spiSettings);

  // First point
  writeReg(RA8876_REG_DLHSR0, x1 & 0xFF);
  writeReg(RA8876_REG_DLHSR1, x1 >> 8);
  writeReg(RA8876_REG_DLVSR0, y1 & 0xFF);
  writeReg(RA8876_REG_DLVSR1, y1 >> 8);

  // Second point
  writeReg(RA8876_REG_DLHER0, x2 & 0xFF);
  writeReg(RA8876_REG_DLHER1, x2 >> 8);
  writeReg(RA8876_REG_DLVER0, y2 & 0xFF);
  writeReg(RA8876_REG_DLVER1, y2 >> 8);

  // Colour
  writeReg(RA8876_REG_FGCR, color >> 11 << 3);
  writeReg(RA8876_REG_FGCG, ((color >> 5) & 0x3F) << 2);
  writeReg(RA8876_REG_FGCB, (color & 0x1F) << 3);

  // Draw
  writeReg(RA8876_REG_DCR1, 0xE0);  // Start drawing, filled, square

  // Wait for completion
  uint8_t status = readStatus();
  int iter = 0;
  while (status & 0x08)
  {
    status = readStatus();
    iter++;
  }

  Serial.print(iter); Serial.println(" iterations");
  
  SPI.endTransaction();
}
