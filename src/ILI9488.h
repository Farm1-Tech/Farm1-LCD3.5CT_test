#pragma once

#include <stdint.h>

// Pin define
#define TFT_DC  (17)
#define TFT_CS  (5)
#define TFT_SDA (23)
#define TFT_CLK (18)
#define TFT_BL  (32)
#define TFT_RST (27)

class ILI9488 {
    public: 
        void init() ;
        void drawBitmap(int x_start, int y_start, int x_end, int y_end, uint16_t* color_data) ;
};

