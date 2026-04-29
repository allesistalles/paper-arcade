#define USER_SETUP_INFO "CYD ESP32-2432S028"
#define ILI9341_DRIVER

// CYD TFT pins
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    21

// Touch (shared SPI bus)
#define TOUCH_CS  33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define SMOOTH_FONT

#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000
