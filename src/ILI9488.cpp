#include "ILI9488.h"

#include <Arduino.h>
#include <soc/spi_struct.h>
#include <soc/dport_access.h>
#include <soc/dport_reg.h>

#define LCD_ACTIVE() digitalWrite(TFT_CS, LOW)
#define LCD_INACTIVE() digitalWrite(TFT_CS, HIGH)

#define LCD_BACKLIGHT_ON() digitalWrite(TFT_BL, HIGH)
#define LCD_BACKLIGHT_OFF() digitalWrite(TFT_BL, LOW)

void DirectIOWrite(uint8_t data) {
  for (uint8_t i=7;i>=0;i--) {
    GPIO.out = (GPIO.out & (1 << TFT_SDA)) | 
                  (
                    (((data >> i) & 0x01) << TFT_SDA)
                  ) |
                  (1 << TFT_CLK) // Send data with CLK HIGH
              ;
    GPIO.out_w1tc = 1 << TFT_CLK; // CLK set to LOW
  }
}

void WriteComm(uint8_t data) {
  digitalWrite(TFT_DC, LOW);
  DirectIOWrite(data);
}

void WriteData(uint8_t data) {
  digitalWrite(TFT_DC, HIGH);
  DirectIOWrite(data);
}

volatile spi_dev_t *spi_dev = (volatile spi_dev_t *)(DR_REG_SPI2_BASE);

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Delay between some initialisation commands
#define TFT_INIT_DELAY 0x80 // Not used unless commandlist invoked

// Generic commands used by TFT_eSPI.cpp
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_SLPIN   0x10
#define TFT_SLPOUT  0x11

#define TFT_INVOFF  0x20
#define TFT_INVON   0x21

#define TFT_DISPOFF 0x28
#define TFT_DISPON  0x29

#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C

#define TFT_RAMRD   0x2E

#define TFT_MADCTL  0x36

#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_RGB 0x00
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_SS  0x02
#define TFT_MAD_GS  0x01

void SwitchToGPIOMode() {
  pinMode(TFT_SDA, OUTPUT);
  pinMode(TFT_CLK, OUTPUT);
}

void SwitchToSPIMode() {
  pinMatrixOutAttach(TFT_SDA, VSPID_IN_IDX, false, false);
  pinMatrixOutAttach(TFT_CLK, VSPICLK_OUT_IDX, false, false);
}

void ILI9488::init() {
  // Setup pin
  pinMode(TFT_DC, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(TFT_BL, OUTPUT);
  LCD_BACKLIGHT_OFF(); // Active LOW

  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, LOW);

  // Reset LCD
  digitalWrite(TFT_RST, LOW);
  delay(50);
  digitalWrite(TFT_RST, HIGH);
  delay(300);
  
  // Init LCD
  SwitchToGPIOMode();
  LCD_ACTIVE();

  WriteComm(0xE0);  // Positive Gamma Control
  WriteData(0x00);
  WriteData(0x03);
  WriteData(0x09);
  WriteData(0x08);
  WriteData(0x16);
  WriteData(0x0A);
  WriteData(0x3F);
  WriteData(0x78);
  WriteData(0x4C);
  WriteData(0x09);
  WriteData(0x0A);
  WriteData(0x08);
  WriteData(0x16);
  WriteData(0x1A);
  WriteData(0x0F);

  WriteComm(0XE1);  // Negative Gamma Control
  WriteData(0x00);
  WriteData(0x16);
  WriteData(0x19);
  WriteData(0x03);
  WriteData(0x0F);
  WriteData(0x05);
  WriteData(0x32);
  WriteData(0x45);
  WriteData(0x46);
  WriteData(0x04);
  WriteData(0x0E);
  WriteData(0x0D);
  WriteData(0x35);
  WriteData(0x37);
  WriteData(0x0F);

  WriteComm(0XC0);  // Power Control 1
  WriteData(0x17);
  WriteData(0x15);

  WriteComm(0xC1);  // Power Control 2
  WriteData(0x41);

  WriteComm(0xC5);  // VCOM Control
  WriteData(0x00);
  WriteData(0x12);
  WriteData(0x80);

  WriteComm(TFT_MADCTL);  // Memory Access Control
  WriteData(0x48);           // MX, BGR

  WriteComm(0x3A);  // Pixel Interface Format
  WriteData(0x66);     // 18 bit colour for SPI

  WriteComm(0xB0);  // Interface Mode Control
  WriteData(0x00);

  WriteComm(0xB1);  // Frame Rate Control
  WriteData(0xA0);

  WriteComm(0xB4);  // Display Inversion Control
  WriteData(0x02);

  WriteComm(0xB6);  // Display Function Control
  WriteData(0x02);
  WriteData(0x02);
  WriteData(0x3B);

  WriteComm(0xB7);  // Entry Mode Set
  WriteData(0xC6);

  WriteComm(0xF7);  // Adjust Control 3
  WriteData(0xA9);
  WriteData(0x51);
  WriteData(0x2C);
  WriteData(0x82);

  WriteComm(TFT_SLPOUT);  // Exit Sleep
  delay(120);

  WriteComm(TFT_DISPON);  // Display on
  delay(25);

  // Filp
  WriteComm(TFT_MADCTL);
  WriteData(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_BGR);

  LCD_INACTIVE();

  Serial.println("Setup with Direct IO OK !");

  // Use SPI module
  // Setup SPI
  DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_SPI2_CLK_EN);
  DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_SPI2_RST);

  // spiInitBus
  spi_dev->slave.trans_done = 0;
  spi_dev->slave.slave_mode = 0;
  // spi_dev->misc.val = 0;
  spi_dev->user.val = 0;
  spi_dev->user1.val = 0;
  spi_dev->ctrl.val = 0;
  spi_dev->ctrl1.val = 0;
  spi_dev->ctrl2.val = 0;
  spi_dev->clock.val = 0;
  spi_dev->pin.val = 0;

  // Set SPI Mode 0
  spi_dev->pin.ck_idle_edge = 0;
  spi_dev->user.ck_out_edge = 1;

  // Set MSBFIRST
  spi_dev->ctrl.wr_bit_order = 0;
  spi_dev->ctrl.rd_bit_order = 0;

  // Set Clock
  uint32_t _div = spiFrequencyToClockDiv(40E6); // 40 MHz
  // uint32_t _div = spiFrequencyToClockDiv(1E6); // 100 kHz
  spi_dev->clock.val = _div;

  // Ref : 8.4.6 Access 8-bit I8080/MT6800 LCD in Master Half-Duplex Mode
  // Setup 1 : Add command 0x27
  // spi_dev->user.usr_command = 1; // Use command
  // spi_dev->user2.usr_command_bitlen = 16 - 1; // 16 bit
  // spi_dev->user2.usr_command_value = 0x27; // Command : 0x27
  spi_dev->user.usr_command = 0;
  spi_dev->user2.usr_command_bitlen = 0;
  spi_dev->user2.usr_command_value = 0;

  // Setup 2 : Disable Address
  spi_dev->user.usr_addr = 0; // Disable Address
  spi_dev->user.usr_dummy = 0;
  spi_dev->user.usr_miso = 0; // Disable MISO

  // Setup 3 : Set dataout
  spi_dev->user.usr_mosi = 1; // Enable MISO

  // Config CS delay
  spi_dev->user.cs_setup = 0;
  spi_dev->user.cs_hold = 0;

  LCD_BACKLIGHT_ON();
}

void ILI9488::drawBitmap(int x_start, int y_start, int x_end, int y_end, uint16_t* color_data) {
  SwitchToGPIOMode();

  LCD_ACTIVE();
  WriteComm(0x2A);
  WriteData((x_start >> 8) & 0xFF);
  WriteData(x_start & 0xFF);
  WriteData((x_end >> 8) & 0xFF);
  WriteData(x_end & 0xFF); 		
  WriteComm(0x2B);
  WriteData((y_start >> 8) & 0xFF);
  WriteData(y_start & 0xFF);
  WriteData((y_end >> 8) & 0xFF);
  WriteData(y_end & 0xFF); 		
  
  WriteComm(0x2C);
  digitalWrite(TFT_DC, HIGH);

  ///
  SwitchToSPIMode();

  // Write Color data
  spi_dev->user.usr_command = 0; // No Command
  spi_dev->user.usr_mosi = 1; // Has Data

  // Set data
  uint32_t color_len = (x_end - x_start + 1) * (y_end - y_start + 1);
  uint8_t byte_index = 0;
  for (uint32_t i=0;i<color_len;i++) {
    ((uint8_t*)(spi_dev->data_buf))[byte_index++] = (color_data[i] >> 11) & 0x1F; // R6
    ((uint8_t*)(spi_dev->data_buf))[byte_index++] = (color_data[i] >> 5) & 0x3F;  // G6
    ((uint8_t*)(spi_dev->data_buf))[byte_index++] = (color_data[i] >> 0) & 0x1F;  // B6
    if ((byte_index == 63) || (i == (color_len - 1))) {
      // Set length
      spi_dev->mosi_dlen.usr_mosi_dbitlen = (byte_index * 8) - 1;

      // Action
      spi_dev->cmd.usr = 1;
      while (spi_dev->cmd.usr) ;

      byte_index = 0;
    }
  }

  LCD_INACTIVE();
}