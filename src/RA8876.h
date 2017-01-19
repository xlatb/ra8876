#pragma GCC diagnostic warning "-Wall"

#ifndef RA8876_H
#define RA8876_H

#include <Arduino.h>
#include <SPI.h>

struct SdramInfo
{
  int speed;       // MHz
  int casLatency;  // CAS latency (2 or 3)
  int banks;       // Banks (2 or 4)
  int rowBits;     // Row addressing bits (11-13)
  int colBits;     // Column addressing bits (8-12)
  int refresh;     // Refresh time in microseconds
};

struct DisplayInfo
{
  int      width;     // Display width
  int      height;    // Display height
  
  uint32_t dotClock;  // Pixel clock in kHz
  
  int      hFrontPorch;  // Will be rounded to the nearest multiple of 8
  int      hBackPorch;
  int      hPulseWidth;  // Will be rounded to the nearest multiple of 8
  
  int      vFrontPorch;
  int      vBackPorch;
  int      vPulseWidth;
};

// Data sheet section 6.1.
// Output frequency is: (m_oscClock * (n + 1)) / (2 ** k)
// There is also a PLL parameter named 'm', but it's unclear how its value could ever be non-zero.
//  When it is zero, the divisor is (2 ** 0) = 1, so we simply ignore it.
struct PllParams
{
  uint32_t freq;  // Frequency in kHz
  int      n;     // Multiplier less 1 (range 1..63)
  int      k;     // Divisor power of 2 (range 0..3 for CCLK/MCLK; range 0..7 for SCLK)
};

#define RGB332(r, g, b) (((r) & 0xE0) | (((g) & 0xE0) >> 3) | (((b) & 0xE0) >> 6))
#define RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3))

// 1MHz. TODO: Figure out actual speed to use
// Data sheet section 5.2 says maximum SPI clock is 50MHz.
#define RA8876_SPI_SPEED 1000000

// With SPI, the RA8876 expects an initial byte where the top two bits are meaningful. Bit 7
// is A0, bit 6 is WR#. See data sheet section 7.3.2 and section 19.
// A0: 0 for command/status, 1 for data
// WR#: 0 for write, 1 for read
#define RA8876_DATA_WRITE  0x80
#define RA8876_DATA_READ   0xC0
#define RA8876_CMD_WRITE   0x00
#define RA8876_STATUS_READ 0x40

// Data sheet 19.2: Chip configuration registers
#define RA8876_REG_SRR     0x00  // Software Reset Register
#define RA8876_REG_CCR     0x01  // Chip Configuration Register
#define RA8876_REG_MACR    0x02  // Memory Access Control Register
#define RA8876_REG_ICR     0x03  // Input Control Register
#define RA8876_REG_MRWDP   0x04  // Memory Read/Write Data Port

// Data sheet 19.3: PLL setting registers
#define RA8876_REG_PPLLC1  0x05  // SCLK PLL control register 1
#define RA8876_REG_PPLLC2  0x06  // SCLK PLL control register 2
#define RA8876_REG_MPLLC1  0x07  // MCLK PLL control register 1
#define RA8876_REG_MPLLC2  0x08  // MCLK PLL control register 2
#define RA8876_REG_SPLLC1  0x09  // CCLK PLL control register 1
#define RA8876_REG_SPLLC2  0x0A  // CCLK PLL control register 2

// Data sheet 19.5: LCD display control registers
#define RA8876_REG_MPWCTR  0x10  // Main/PIP Window Control Register
#define RA8876_REG_PIPCDEP 0x11  // PIP Window Color Depth register
#define RA8876_REG_DPCR    0x12  // Display configuration register
#define RA8876_REG_PCSR    0x13  // Panel scan clock and data setting register
#define RA8876_REG_HDWR    0x14  // Horizontal Display Width Register
#define RA8876_REG_HDWFTR  0x15  // Horizontal Display Width Fine Tuning Register
#define RA8876_REG_HNDR    0x16  // Horizontal Non-Display Period Register
#define RA8876_REG_HNDFTR  0x17  // Horizontal Non-Display Period Fine Tuning Register
#define RA8876_REG_HSTR    0x18  // HSYNC start position register
#define RA8876_REG_HPWR    0x19  // HSYNC Pulse Width Register
#define RA8876_REG_VDHR0   0x1A  // Vertical Display Height Register 0
#define RA8876_REG_VDHR1   0x1B  // Vertical Display Height Register 1
#define RA8876_REG_VNDR0   0x1C  // Vertical Non-Display Period Register 0
#define RA8876_REG_VNDR1   0x1D  // Vertical Non-Display Period Register 1
#define RA8876_REG_VSTR    0x1E  // VSYNC start position register
#define RA8876_REG_VPWR    0x1F  // VSYNC pulse width register
#define RA8876_REG_MISA0   0x20  // Main Image Start Address 0
#define RA8876_REG_MISA1   0x21  // Main Image Start Address 1
#define RA8876_REG_MISA2   0x22  // Main Image Start Address 2
#define RA8876_REG_MISA3   0x23  // Main Image Start Address 3
#define RA8876_REG_MIW0    0x24  // Main Image Width 0
#define RA8876_REG_MIW1    0x25  // Main Image Width 1
#define RA8876_REG_MWULX0  0x26  // Main Window Upper-Left X coordinate 0
#define RA8876_REG_MWULX1  0x27  // Main Window Upper-Left X coordinate 1
#define RA8876_REG_MWULY0  0x28  // Main Window Upper-Left Y coordinate 0
#define RA8876_REG_MWULY1  0x29  // Main Window Upper-Left Y coordinate 1

// Data sheet 19.6: Geometric engine control registers
#define RA8876_REG_CVSSA0     0x50  // Canvas Start Address 0
#define RA8876_REG_CVSSA1     0x51  // Canvas Start Address 1
#define RA8876_REG_CVSSA2     0x52  // Canvas Start Address 2
#define RA8876_REG_CVSSA3     0x53  // Canvas Start Address 3
#define RA8876_REG_CVS_IMWTH0 0x54  // Canvas image width 0
#define RA8876_REG_CVS_IMWTH1 0x55  // Canvas image width 1
#define RA8876_REG_AWUL_X0    0x56  // Active Window Upper-Left X coordinate 0
#define RA8876_REG_AWUL_X1    0x57  // Active Window Upper-Left X coordinate 1
#define RA8876_REG_AWUL_Y0    0x58  // Active Window Upper-Left Y coordinate 0
#define RA8876_REG_AWUL_Y1    0x59  // Active Window Upper-Left Y coordinate 1
#define RA8876_REG_AW_WTH0    0x5A  // Active Window Width 0
#define RA8876_REG_AW_WTH1    0x5B  // Active Window Width 1
#define RA8876_REG_AW_HT0     0x5C  // Active Window Height 0
#define RA8876_REG_AW_HT1     0x5D  // Active Window Height 1
#define RA8876_REG_AW_COLOR   0x5E  // Color Depth of canvas & active window
#define RA8876_REG_CURH0      0x5F  // Graphic read/write horizontal position 0
#define RA8876_REG_CURH1      0x60  // Graphic read/write horizontal position 1
#define RA8876_REG_CURV0      0x61  // Graphic read/write vertical position 0
#define RA8876_REG_CURV1      0x62  // Graphic read/write vertical position 1

#define RA8876_REG_DLHSR0     0x68  // Draw shape point 1 X coordinate register 0
#define RA8876_REG_DLHSR1     0x69  // Draw shape point 1 X coordinate register 1
#define RA8876_REG_DLVSR0     0x6A  // Draw shape point 1 Y coordinate register 0
#define RA8876_REG_DLVSR1     0x6B  // Draw shape point 1 Y coordinate register 1

#define RA8876_REG_DLHER0     0x6C  // Draw shape point 2 X coordinate register 0
#define RA8876_REG_DLHER1     0x6D  // Draw shape point 2 X coordinate register 1
#define RA8876_REG_DLVER0     0x6E  // Draw shape point 2 Y coordinate register 0
#define RA8876_REG_DLVER1     0x6F  // Draw shape point 2 Y coordinate register 1

#define RA8876_REG_DTPH0      0x70  // Draw shape point 3 X coordinate register 0
#define RA8876_REG_DTPH1      0x71  // Draw shape point 3 X coordinate register 1
#define RA8876_REG_DTPV0      0x72  // Draw shape point 3 Y coordinate register 0
#define RA8876_REG_DTPV1      0x73  // Draw shape point 3 Y coordinate register 1

#define RA8876_REG_DCR1       0x76  // Draw shape control register 1

#define RA8876_REG_FGCR       0xD2  // Foreground colour register - red
#define RA8876_REG_FGCG       0xD3  // Foreground colour register - green
#define RA8876_REG_FGCB       0xD4  // Foreground colour register - blue

// Data sheet 19.7: PWM timer control registers
#define RA8876_REG_PSCLR      0x84  // PWM prescaler register
#define RA8876_REG_PMUXR      0x85  // PWM clock mux register
#define RA8876_REG_PCFGR      0x86  // PWM configuration register

// Data sheet 19.12: SDRAM control registers
#define RA8876_REG_SDRAR         0xE0  // SDRAM attribute register
#define RA8876_REG_SDRMD         0xE1  // SDRAM mode & extended mode register
#define RA8876_REG_SDR_REF_ITVL0 0xE2  // SDRAM auto refresh interval 0
#define RA8876_REG_SDR_REF_ITVL1 0xE3  // SDRAM auto refresh interval 1
#define RA8876_REG_SDRCR         0xE4  // SDRAM control register


class RA8876
{
private:
  int m_csPin;
  int m_resetPin;

  int m_width;
  int m_height;
  int m_depth;

  uint32_t m_oscClock;   // OSC clock (external crystal) frequency in kHz

  PllParams m_memPll;   // MCLK (memory) PLL parameters
  PllParams m_corePll;  // CCLK (core) PLL parameters
  PllParams m_scanPll;  // SCLK (LCD panel scan) PLL parameters

  SPISettings m_spiSettings;

  SdramInfo *m_sdramInfo;
  
  DisplayInfo *m_displayInfo;

  void hardReset(void);
  void softReset(void);

  void writeCmd(uint8_t x);
  void writeData(uint8_t x);
  uint8_t readData(void);
  uint8_t readStatus(void);

  void writeReg(uint8_t reg, uint8_t x);
  uint8_t readReg(uint8_t reg);

  bool calcPllParams(uint32_t targetFreq, int kMax, PllParams *pll);
  bool calcClocks(void);
  void dumpClocks(void);

  bool initPLL(void);
  bool initMemory(SdramInfo *info);
  bool initDisplay(void);
public:
  RA8876(int csPin, int resetPin = 0);

  // Init
  bool init(void);
  
  // Dimensions
  int getWidth() { return m_width; };
  int getHeight() { return m_height; };

  // Test
  void colorBarTest(bool enabled);
  
  // Drawing
  void drawPixel(int x, int y, uint16_t color);

  void fillRect(int x1, int y1, int x2, int y2, uint16_t color);
};

#endif
