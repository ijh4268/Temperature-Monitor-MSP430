/**
 * Simplified shell of an OLED driver for SPI version of the SSD1306
 * These displays are very common on Adafruit, Sparkfun, Amazon, Ebay, and AllieExpress
 *
 * @author Josiah Hester (adapted from various Adafruit libraries)
 */
#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

// These were grabbed from the Adafruit module
#define SSD1306_BLACK               0    ///< Draw 'off' pixels
#define SSD1306_WHITE               1    ///< Draw 'on' pixels
#define SSD1306_INVERSE             2    ///< Invert pixels

#define SSD1306_MEMORYMODE          0x20 ///< See datasheet
#define SSD1306_COLUMNADDR          0x21 ///< See datasheet
#define SSD1306_PAGEADDR            0x22 ///< See datasheet
#define SSD1306_SETCONTRAST         0x81 ///< See datasheet
#define SSD1306_CHARGEPUMP          0x8D ///< See datasheet
#define SSD1306_SEGREMAP            0xA0 ///< See datasheet
#define SSD1306_DISPLAYALLON_RESUME 0xA4 ///< See datasheet
#define SSD1306_DISPLAYALLON        0xA5 ///< Not currently used
#define SSD1306_NORMALDISPLAY       0xA6 ///< See datasheet
#define SSD1306_INVERTDISPLAY       0xA7 ///< See datasheet
#define SSD1306_SETMULTIPLEX        0xA8 ///< See datasheet
#define SSD1306_DISPLAYOFF          0xAE ///< See datasheet
#define SSD1306_DISPLAYON           0xAF ///< See datasheet
#define SSD1306_COMSCANINC          0xC0 ///< Not currently used
#define SSD1306_COMSCANDEC          0xC8 ///< See datasheet
#define SSD1306_SETDISPLAYOFFSET    0xD3 ///< See datasheet
#define SSD1306_SETDISPLAYCLOCKDIV  0xD5 ///< See datasheet
#define SSD1306_SETPRECHARGE        0xD9 ///< See datasheet
#define SSD1306_SETCOMPINS          0xDA ///< See datasheet
#define SSD1306_SETVCOMDETECT       0xDB ///< See datasheet

#define SSD1306_SETLOWCOLUMN        0x00 ///< Not currently used
#define SSD1306_SETHIGHCOLUMN       0x10 ///< Not currently used
#define SSD1306_SETSTARTLINE        0x40 ///< See datasheet

#define SSD1306_EXTERNALVCC         0x01 ///< External display voltage source
#define SSD1306_SWITCHCAPVCC        0x02 ///< Gen. display voltage from 3.3V

#define SSD1306_RIGHT_HORIZONTAL_SCROLL              0x26 ///< Init rt scroll
#define SSD1306_LEFT_HORIZONTAL_SCROLL               0x27 ///< Init left scroll
#define SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL 0x29 ///< Init diag scroll
#define SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL  0x2A ///< Init diag scroll
#define SSD1306_DEACTIVATE_SCROLL                    0x2E ///< Stop scroll
#define SSD1306_ACTIVATE_SCROLL                      0x2F ///< Start scroll
#define SSD1306_SET_VERTICAL_SCROLL_AREA             0xA3 ///< Set scroll range

#define FONT_LARGE 0
#define FONT_MEDIUM 1
#define FONT_SMALL 2

class SSD1306 {
public:
    // Functions for display control
    void init();
    void refresh();
    void clear(uint8_t color);
    void clear_on();
    void clear_off();
    void set_xy(uint8_t x, uint8_t y, uint8_t color);
    bool get_xy(int16_t x, int16_t y);
    uint8_t * buffer_ptr();

    // Drawing functions
    void draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
    void fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
    void draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
    void draw_triangle();
    void fill_triangle();

    // Fast drawing functions
    void drawFastVLineInternal(int16_t x, int16_t __y, int16_t __h, uint8_t color);
    void drawFastHLineInternal(int16_t x, int16_t y, int16_t w, uint16_t color);

    // Text
    void drawChar(int16_t x, int16_t y, unsigned char c,
                          uint16_t color, uint16_t bg, uint8_t size);
    void drawText(uint8_t x, uint8_t y, char *line, uint8_t textcolor);
    void setFont(uint8_t size);

    void drawImage(const uint8_t * image, uint8_t x, uint8_t y);

};


#endif /* OLED_H_ */
