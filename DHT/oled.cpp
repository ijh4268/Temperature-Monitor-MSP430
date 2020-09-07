/**
 * Simplified shell of an OLED driver for SPI version of the SSD1306
 * These displays are very common on Adafruit, Sparkfun, Amazon, Ebay, and AllieExpress
 *
 * @author Josiah Hester (adapted from various Adafruit libraries)
 */
#include "oled.h"
#include "gfxfont.h"
#include "fonts/SourceSansProBlack15pt.h"
#include "fonts/SourceSansProBold10pt.h"
#include "fonts/SourceSansProBold7pt.h"
#include <msp430.h>
#include <string.h>
#include <math.h>

#define DISPLAY_IO_PORT P8OUT
#define SPI_CS BIT1
#define DISPLAY_DC BIT2
#define DISPLAY_RST BIT3


#define SSD1306_SELECT       DISPLAY_IO_PORT  &= ~SPI_CS;  ///< Device select
#define SSD1306_DESELECT     DISPLAY_IO_PORT  |= SPI_CS; ///< Device deselect
#define SSD1306_MODE_COMMAND DISPLAY_IO_PORT  &= ~DISPLAY_DC;  ///< Command mode
#define SSD1306_MODE_DATA    DISPLAY_IO_PORT  |= DISPLAY_DC; ///< Data mode

#define WIDTH 128
#define HEIGHT 64
#define BUFFER_SIZE WIDTH * ((HEIGHT + 7) / 8)
uint8_t buffer[BUFFER_SIZE];
//uint8_t buffer[HEIGHT][WIDTH/8];

// The default font
GFXfont *gfxFont;

// PRIVATE ---------------------------------------------------------

void init_spi() {
    // Configure GPIO
    P5SEL1 &= ~(BIT0 | BIT1 | BIT2);        // USCI_B1 SCLK, MOSI, and MISO pin
    P5SEL0 |= (BIT0 | BIT1 | BIT2);

    // Configure USCI_B1 for SPI operation
    UCB1CTLW0 = UCSWRST;                    // **Put state machine in reset**
    UCB1CTLW0 |= UCMST | UCSYNC | UCCKPL | UCMSB; // 3-pin, 8-bit SPI master
                                            // Clock polarity high, MSB
    UCB1CTLW0 |= UCSSEL__SMCLK;              // SMCLK
    UCB1CTLW0 &= ~UCSWRST;                  // **Initialize USCI state machine**
}

void spi_write(uint8_t data) {
    UCB1TXBUF = data;                   // Transmit characters
}

void ssd1306_command(uint8_t cmd) {
    SSD1306_MODE_COMMAND
    spi_write(cmd);
}


// API ---------------------------------------------------------

void SSD1306::clear_off() {
    memset(buffer, 0, BUFFER_SIZE);

}

void SSD1306::clear_on() {
    memset(buffer, 255, BUFFER_SIZE);
}

void SSD1306::clear(uint8_t color) {
    switch(color) {
             case SSD1306_WHITE:   memset(buffer, 255, BUFFER_SIZE); break;
             case SSD1306_BLACK:   memset(buffer, 0, BUFFER_SIZE); break;
    }
}

void SSD1306::init(void) {
    init_spi();

    // Init the other pins (Chip Select, Data Command)
    P8DIR |= SPI_CS + DISPLAY_DC + DISPLAY_RST;

    // Deselect and set DC to command
    SSD1306_DESELECT
    SSD1306_MODE_COMMAND

    // Handle the pin hardware reset
    // Reset SSD1306 using reset pin
    DISPLAY_IO_PORT  |= DISPLAY_RST;
    __delay_cycles(1000);       // VDD goes high at start, pause for 1 ms
    DISPLAY_IO_PORT  &= ~DISPLAY_RST;  // Bring reset low
    __delay_cycles(10000);      // Wait 10 ms
    DISPLAY_IO_PORT  |= DISPLAY_RST; // Bring out of reset

    SSD1306_SELECT

    ssd1306_command(SSD1306_DISPLAYOFF);           // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);   // 0xD5
    ssd1306_command(0x80);                         // the suggested ratio 0x80
    ssd1306_command(SSD1306_SETMULTIPLEX);         // 0xA8
    ssd1306_command(HEIGHT - 1);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);     // 0xD3
    ssd1306_command(0x0);                          // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);   // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);           // 0x8D
    ssd1306_command(0x14);                         // generate high voltage from 3.3v line internally
    ssd1306_command(SSD1306_MEMORYMODE);            // 0x20
    ssd1306_command(0x00);                         // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);

    uint8_t comPins = 0x02;
    uint8_t contrast = 0x8F;

    if((WIDTH == 128) && (HEIGHT == 32)) {
      comPins = 0x02;
      contrast = 0x8F;
    } else if((WIDTH == 128) && (HEIGHT == 64)) {
      comPins = 0x12;
      contrast = 0xCF;
    }

    ssd1306_command(SSD1306_SETCOMPINS);
    ssd1306_command(comPins);
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(contrast);

    ssd1306_command(SSD1306_SETPRECHARGE); // 0xd9
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);         // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);         // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);               // 0xA6
    ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
    ssd1306_command(SSD1306_DISPLAYON);

    // Finish transaction (if SPI bus is not shared, then just hold CS to GND in breadboard)
    SSD1306_DESELECT

    // Select the font
    //gfxFont = (GFXfont *) &FreeSans12pt7b;
    gfxFont = (GFXfont *) &SourceSansPro_Black15pt7b;
}

/*!
    @brief  Push data currently in RAM to SSD1306 display.
    @return None (void).
    @note   Drawing operations are not visible until this function is
            called. Call after each graphics command, or after a whole set
            of graphics commands, as best needed by one's own application.
*/
void SSD1306::refresh() {
    SSD1306_SELECT
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);                         // Page start address
    ssd1306_command(0xFF);                      // Page end (not really, but works here)
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);                       // Column start address
    ssd1306_command(WIDTH - 1); // Column end address

    uint16_t count = WIDTH * ((HEIGHT + 7) / 8);
    unsigned char *pucData = buffer;
    SSD1306_MODE_DATA
    while(count--) spi_write(*pucData++);
    SSD1306_DESELECT
}

void SSD1306::set_xy(uint8_t x, uint8_t y, uint8_t color) {
    // Bound the pixels, if you write to anything larger then the display freaks out
    uint16_t ndx = x + (y>>3)*WIDTH;
    if(y < HEIGHT-1 || x < WIDTH - 1) {
        switch(color) {
         case SSD1306_WHITE:  buffer[ndx] |=  (1 << (y&7)); break;
         case SSD1306_BLACK:  buffer[ndx] &= ~(1 << (y&7)); break;
        }
    }
}

void SSD1306::drawText(uint8_t x, uint8_t y, char *line, uint8_t textcolor) {
  uint8_t cursor_x = x;
  uint8_t cursor_y = y;
  for (uint8_t j=0; j<strlen(line); j++) {
    char c = line[j];
    if(c == '\n') {
      cursor_x  = 0;
      cursor_y += gfxFont->yAdvance;
    } else if(c != '\r') {
      uint8_t first = gfxFont->first;
      if((c >= first) && (c <= gfxFont->last)) {
        uint8_t   c2    = c - gfxFont->first;
        GFXglyph *glyph = &(gfxFont->glyph)[c2];
        uint8_t   w     = glyph->width,
                  h     = glyph->height;
        if((w > 0) && (h > 0)) { // Is there an associated bitmap?
          int16_t xo = glyph->xOffset; // sic
          if(((cursor_x + (xo + w)) >= WIDTH)) {
            // Drawing character would go off right edge; wrap to new line
            cursor_x  = 0;
            cursor_y += gfxFont->yAdvance;
          }
          drawChar(cursor_x, cursor_y, c, textcolor, 0, 1);
        }
        cursor_x += glyph->xAdvance;
      }
    }
  }
}

// Draw a character
/**************************************************************************/
/*!
   @brief   Draw a single character
    @param    x   Bottom left corner x coordinate
    @param    y   Bottom left corner y coordinate
    @param    c   The 8-bit font-indexed character (likely ascii)
    @param    color 16-bit 5-6-5 Color to draw chraracter with
    @param    bg 16-bit 5-6-5 Color to fill background with (if same as color,
   no background)
    @param    size_x  Font magnification level in X-axis, 1 is 'original' size
    @param    size_y  Font magnification level in Y-axis, 1 is 'original' size
*/
/**************************************************************************/
void SSD1306::drawChar(int16_t x, int16_t y, unsigned char c,
 uint16_t color, uint16_t bg, uint8_t size) {

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling drawChar()
    // directly with 'bad' characters of font may cause mayhem!

    c -= gfxFont->first;
    GFXglyph *glyph  = &(gfxFont->glyph)[c];
    uint8_t  *bitmap = gfxFont->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width,
             h  = glyph->height,
             xa = glyph->xAdvance;
    int8_t   xo = glyph->xOffset,
             yo = glyph->yOffset + gfxFont->yAdvance;
    uint8_t  xx=0, yy=0, bits=0, bit = 0;
    int16_t  xo16=0, yo16=0;

    if(size > 1) {
      xo16 = xo;
      yo16 = yo;
    }

    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if(bits & 0x80) {
          if(size == 1) {
            set_xy(x+xo+xx, y+yo+yy, color);
          } else {
            fill_rect(x+(xo16+xx)*size, y+(yo16+yy)*size, size, size, color);
          }
        }
        //bits <<= 1;
        bits = bits << 1;
      }
    }
}

/*!
 *  These are borrowed from Adafruit SSD1306 library
 *  https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp
    @brief  Draw a horizontal line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Leftmost column -- 0 at left to (screen width - 1) at right.
    @param  y
            Row of display -- 0 at top to (screen height -1) at bottom.
    @param  w
            Width of line, in pixels.
    @param  color
            Line color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306::drawFastHLineInternal(
  int16_t x, int16_t y, int16_t w, uint16_t color) {

  if((y >= 0) && (y < HEIGHT)) { // Y coord in bounds?
    if(x < 0) { // Clip left
      w += x;
      x  = 0;
    }
    if((x + w) > WIDTH) { // Clip right
      w = (WIDTH - x);
    }
    if(w > 0) { // Proceed only if width is positive
      uint8_t *pBuf = &buffer[(y / 8) * WIDTH + x],
               mask = 1 << (y & 7);
      switch(color) {
       case SSD1306_WHITE:               while(w--) { *pBuf++ |= mask; }; break;
       case SSD1306_BLACK: mask = ~mask; while(w--) { *pBuf++ &= mask; }; break;
       case SSD1306_INVERSE:             while(w--) { *pBuf++ ^= mask; }; break;
      }
    }
  }
}

/*!
 *  These are borrowed from Adafruit SSD1306 library
 *  https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp
    @brief  Draw a vertical line. This is also invoked by the Adafruit_GFX
            library in generating many higher-level graphics primitives.
    @param  x
            Column of display -- 0 at left to (screen width -1) at right.
    @param  y
            Topmost row -- 0 at top to (screen height - 1) at bottom.
    @param  h
            Height of line, in pixels.
    @param  color
            Line color, one of: SSD1306_BLACK, SSD1306_WHITE or SSD1306_INVERT.
    @return None (void).
    @note   Changes buffer contents only, no immediate effect on display.
            Follow up with a call to display(), or with other graphics
            commands as needed by one's own application.
*/
void SSD1306::drawFastVLineInternal(int16_t x, int16_t __y, int16_t __h, uint8_t color) {

  if((x >= 0) && (x < WIDTH)) { // X coord in bounds?
    if(__y < 0) { // Clip top
      __h += __y;
      __y = 0;
    }
    if((__y + __h) > HEIGHT) { // Clip bottom
      __h = (HEIGHT - __y);
    }
    if(__h > 0) { // Proceed only if height is now positive
      // this display doesn't need ints for coordinates,
      // use local byte registers for faster juggling
      uint8_t  y = __y, h = __h;
      uint8_t *pBuf = &buffer[(y >> 3) * WIDTH + x];

      // do the first partial byte, if necessary - this requires some masking
      uint8_t mod = (y & 7);
      if(mod) {
        // mask off the high n bits we want to set
        mod = 8 - mod;
        // note - lookup table results in a nearly 10% performance
        // improvement in fill* functions
        // uint8_t mask = ~(0xFF >> mod);
        static const uint8_t premask[8] =
          { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };
        uint8_t mask = *(const unsigned char *)(&premask[mod]);
        // adjust the mask if we're not going to reach the end of this byte
        if(h < mod) mask &= (0XFF >> (mod - h));

        switch(color) {
         case SSD1306_WHITE:   *pBuf |=  mask; break;
         case SSD1306_BLACK:   *pBuf &= ~mask; break;
         case SSD1306_INVERSE: *pBuf ^=  mask; break;
        }
        pBuf += WIDTH;
      }

      if(h >= mod) { // More to go?
        h -= mod;
        // Write solid bytes while we can - effectively 8 rows at a time
        if(h >= 8) {
          if(color == SSD1306_INVERSE) {
            // separate copy of the code so we don't impact performance of
            // black/white write version with an extra comparison per loop
            do {
              *pBuf ^= 0xFF;  // Invert byte
              pBuf  += WIDTH; // Advance pointer 8 rows
              h     -= 8;     // Subtract 8 rows from height
            } while(h >= 8);
          } else {
            // store a local value to work with
            uint8_t val = (color != SSD1306_BLACK) ? 255 : 0;
            do {
              *pBuf = val;    // Set byte
              pBuf += WIDTH;  // Advance pointer 8 rows
              h    -= 8;      // Subtract 8 rows from height
            } while(h >= 8);
          }
        }

        if(h) { // Do the final partial byte, if necessary
          mod = h & 7;
          // this time we want to mask the low bits of the byte,
          // vs the high bits we did above
          // uint8_t mask = (1 << mod) - 1;
          // note - lookup table results in a nearly 10% performance
          // improvement in fill* functions
          static const uint8_t postmask[8] =
            { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F };
          uint8_t mask = *(const unsigned char *)(&postmask[mod]);
          switch(color) {
           case SSD1306_WHITE:   *pBuf |=  mask; break;
           case SSD1306_BLACK:   *pBuf &= ~mask; break;
           case SSD1306_INVERSE: *pBuf ^=  mask; break;
          }
        }
      }
    } // endif positive height
  } // endif x in bounds
}

void SSD1306::setFont(uint8_t size) {
    switch(size) {
        case FONT_LARGE: gfxFont = (GFXfont *) &SourceSansPro_Black15pt7b; break;
        case FONT_MEDIUM: gfxFont = (GFXfont *) &SourceSansPro_Bold10pt7b; break;
        case FONT_SMALL: gfxFont = (GFXfont *) &SourceSansPro_Bold7pt7b; break;
    }
}

uint8_t * SSD1306::buffer_ptr() {
    return buffer;
}


// drawImage
// Draw an Image on the display
// Image can be generated with ImageDog (check for ImageDog on GitHub)
// Header : width in pixel
//          hight in pixel/8
//
void SSD1306::drawImage(const uint8_t * image, uint8_t x,uint8_t y) {
    // height in rows (row = 8 pixels), width in columns
    volatile uint8_t imgheight, imgwidth;
    volatile uint8_t data=0;
    imgwidth = *image++;
    imgheight = (*image++)*8;

    for (uint8_t a = 0; a < imgheight; a+=8)
    {
      data=0;
      for (uint8_t i=0; i<imgwidth; i++) {
        data  = *image++;
        for (uint8_t j=0; j<8; j++) {
          data & 0x80 ? set_xy(x+i,y+a+j,1) : set_xy(x+i,y+a+j,0);
          data <<= 1;
        }
      }
    }
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************


bool SSD1306::get_xy(int16_t x, int16_t y) {
    uint16_t ndx = x + (y>>3)*WIDTH;
    if(y < HEIGHT - 1 || x < WIDTH - 1) {
        return (buffer[ndx] & (1 << (y & 7)));
    }
    return false; // Pixel out of bounds
}

void SSD1306::draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    drawFastVLineInternal(x, y, height, color);
    drawFastVLineInternal(x + width - 1, y, height, color);
    drawFastHLineInternal(x, y, width, color);
    drawFastHLineInternal(x, y + height - 1, width, color);
}

void SSD1306::fill_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color) {
    for(int i = 0; i < width; i++){
        drawFastVLineInternal(x + i, y, height, color);
    }
}

void SSD1306::draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color) {
    int x = x1;
    int y = y1;
    int dx = x2 - x1;
    int dy = y2 - y1;

    int p = 2*dy - dx;

    while(x <= x2){
        set_xy(x, y, SSD1306_WHITE);
        x++;
        if(p < 0){
            p = p + 2*dy;
        }
        else{
            p = p + 2*dy - 2*dx;
            y++;
        }
    }
}

