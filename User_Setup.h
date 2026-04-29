#define USER_SETUP_INFO "CYD ESP32-2432S028"
#define ILI9341_2_DRIVER

// CYD TFT pins (HSPI bus)
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    21
#define TFT_BACKLIGHT_ON HIGH

// Force TFT_eSPI to use HSPI explicitly. Touch is on a separate VSPI bus,
// initialized in paper-arcade.ino with pins CLK=25, MOSI=32, MISO=39.
#define USE_HSPI_PORT

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY        55000000
#define SPI_READ_FREQUENCY   20000000
