#include <Arduino.h>
#include <Wire.h>
#include <demos/lv_demos.h>
#include <lvgl.h>
#include "ILI9488.h"

/*Change to your screen resolution*/
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

#define TOUCH_ADDR (0x38)

ILI9488 tft;

bool reg_read(uint8_t addr, uint8_t *data, size_t len) {
    Wire1.beginTransmission(TOUCH_ADDR);
    Wire1.write(addr);
    Wire1.endTransmission(false);

    int n = Wire1.requestFrom(TOUCH_ADDR, len);
    if (n != len) {
        Serial.printf("read fail code %d\n", n);
        return false;
    }
    Wire1.readBytes(data, len);

    return true;
}

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    tft.drawBitmap(area->x1, area->y1, area->x2, area->y2, &color_p->full);

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint8_t touch_point = 0;
    if (reg_read(0x02, &touch_point, 1)) {
        if (touch_point > 0) {
            struct {
                uint8_t XH;
                uint8_t XL;
                uint8_t YH;
                uint8_t YL;
            } P1;
            if (reg_read(0x03, (uint8_t *)&P1, sizeof(P1))) {
                uint16_t x = ((uint16_t)(P1.XH & 0x0F) << 8) | P1.XL;
                uint16_t y = ((uint16_t)(P1.YH & 0x0F) << 8) | P1.YL;
                // Serial.printf("Touch %d, %d\n", x, y);

                y = map(y, 30, 480, 0, 480);
                data->point.x = 480 - y;
                data->point.y = x;
                data->state = LV_INDEV_STATE_PR;
            } else {
                Serial.println("read point 1 error");
                data->state = LV_INDEV_STATE_REL;
            }
        }
    } else {
        Serial.println("read point count error");
        data->state = LV_INDEV_STATE_REL;
    }

    return;
}

void setup() {
    Serial.begin(115200); /* prepare for possible serial debug */

    Wire1.begin(19, 16, 100E3);

    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);
    Serial.println("I am LVGL_Arduino");

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    tft.init();
    /*uint16_t c[20 * 20];
    for (int i=0;i<(20*20);i++) {
        c[i] = 0xFFFF;
    }
    tft.drawBitmap(0, 0, 20, 20, c);
    while(1) delay(100);*/

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

#if 0
    /* Create simple label */
    lv_obj_t *label = lv_label_create( lv_scr_act() );
    lv_label_set_text( label, LVGL_Arduino.c_str() );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );
#else
    /* Try an example from the lv_examples Arduino library
       make sure to include it as written above.
    lv_example_btn_1();
   */

    // uncomment one of these demos
    lv_demo_widgets();  // OK
                        // lv_demo_benchmark();          // OK
                        // lv_demo_keypad_encoder();     // works, but I haven't an encoder
                        // lv_demo_music();              // NOK
                        // lv_demo_printer();
                        // lv_demo_stress();             // seems to be OK
#endif
    Serial.println("Setup done");
}

void loop() {
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}
