/*
 * DHT.c
 *
 *  Created on: Aug 24, 2020
 *      Author: ijh19
 *      This code is largely borrowed from https://gist.github.com/DmitryMyadzelets/23b98a183622b5ed6b9c
 *      with some adjustments to work with the MSP430FR5994
 */
#include <msp430.h>
#include <string.h>
#include "oled.h"

#include "images/sun.h"

volatile unsigned txdata = 0;

SSD1306 display;

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer0_A0(void)
{
    if (txdata)
    {
        (txdata & 1) ? (TA0CCTL0 &= ~OUTMOD2) : (TA0CCTL0 |= OUTMOD2);
        txdata >>= 1;
        TA0CCR0 += 104;
    }
    else
    {
        TA0CCTL0 &= ~CCIE;
    }
}

void init_clocks()
{
    // Startup clock system with max DCO setting ~8MHz
    CSCTL0_H = CSKEY_H;                     // Unlock CS registers
    CSCTL1 = DCOFSEL_3 | DCORSEL;           // Set DCO to 8MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
    CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;   // Set all dividers
    CSCTL0_H = 0;                           // Lock CS registers
}

int read_dht(unsigned char *p)
{
    // Note: TimerA must be continuous mode (MC_2) at 1 MHz
    const unsigned b = BIT4;                                    // I/O bit
    const unsigned char *end = p + 6;                      // End of data buffer
    register unsigned char m = 1;         // First byte will have only start bit
    register unsigned st, et;                             // Start and end times
                                                          //
    p[0] = p[1] = p[2] = p[3] = p[4] = p[5] = 0;            // Clear data buffer
                                                            //
    P1OUT &= ~b;                                                // Pull low
    P1DIR |= b;                                                 // Output
    P1REN &= ~b;                                                // Drive low
    st = TA0R;
    while ((TA0R - st) < 18000)
        ;                      // Wait 18 ms
    P1REN |= b;                                                 // Pull low
    P1OUT |= b;                                                 // Pull high
    P1DIR &= ~b;                                                // Input
                                                                //
    st = TA0R;                                     // Get start time for timeout
    while (P1IN & b)
        if ((TA0R - st) > 100)
            return -1;             // Wait while high, return if no response
    et = TA0R;                                     // Get start time for timeout
    do
    {                                                        //
        st = et;           // Start time of this bit is end time of previous bit
        while (!(P1IN & b))
            if ((TA0R - st) > 100)
                return -2;      // Wait while low, return if stuck low
        while (P1IN & b)
            if ((TA0R - st) > 200)
                return -3;         // Wait while high, return if stuck high
        et = TA0R;                                               // Get end time
        if ((et - st) > 110)
            *p |= m;                   // If time > 110 us, then it is a one bit
        if (!(m >>= 1))
            m = 0x80, ++p;    // Shift mask, move to next byte when mask is zero
    }
    while (p < end);                                   // Do until array is full
                                                       //
    p -= 6;                                          // Point to start of buffer
    if (p[0] != 1)
        return -4;                                    // No start bit
    if (((p[1] + p[2] + p[3] + p[4]) & 0xFF) != p[5])
        return -5; // Bad checksum
                   //
    return 0;                                                   // Good read
}

void itoa(unsigned int value, char *result, int base)
{
// check that the base if valid
    if (base < 2 || base > 36)
    {
        *result = '\0';
    }

    char *ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do
    {
        tmp_value = value;
        value /= base;
        *ptr++ =
                "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35
                        + (tmp_value - value * base)];
    }
    while (value);

// Apply negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

void main(void)
{
    // Disable the GPIO power-on default high-impedance mode to activate
    // previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    unsigned char dht[6];    // Data from DHT11
    int err;    //

    const int x = 88;
    const int y = 18;
    //
    WDTCTL = WDTPW | WDTHOLD;    // Disable watchdog reset
    init_clocks();
    TA0CTL = TASSEL_2 | MC_2 | ID_3;    // TimerA SMCLK, continuous, divide by 8 to get 1MHz
    __enable_interrupt();

    display.init();
    display.clear_off();

    for (;;)
    {                                       // for-ever
        while (TA0CCTL0 & CCIE)
            ;                                // Wait for any serial tx to finish
        err = read_dht(dht);                                       // Read DHT11
        display.clear(SSD1306_BLACK);
        display.draw_rect(15, 15, 110, 38, SSD1306_WHITE);
        char *title = "Current Temp:";
        display.drawImage(sun_image, x, y);
        display.setFont(FONT_SMALL);
        display.drawText(0, 0, title, SSD1306_WHITE);
        display.setFont(FONT_LARGE);
        if (err)
        {
            display.setFont(FONT_MEDIUM);
            char txt[10] = "Error";
            display.drawText(20, 22, txt, SSD1306_WHITE);
        }
        else
        {
            char txt[80] = "";
            int temp = dht[3] * (1.8) + 32.0; // convert from C to F
            itoa(temp, txt, 10);
            strcat(txt, " F");
            display.drawText(20, 22, txt, SSD1306_WHITE);
        }
        display.refresh();
        __delay_cycles(10000000);                    // Wait a while
    }                                               //
}

