#pragma once
// TFT_eSPI is used ONLY as a RAM sprite/font renderer. Its SPI driver is never
// initialized. ST77922Panel owns the actual QSPI interface and initialization.
#define ILI9341_DRIVER
#define TFT_MISO -1
#define TFT_MOSI -1
#define TFT_SCLK -1
#define TFT_CS -1
#define TFT_DC -1
#define TFT_RST -1
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT7
#define LOAD_FONT8
