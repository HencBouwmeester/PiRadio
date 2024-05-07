/*
 * Fixed width font for numbers
 *
 * This font is very useful when using overstrike as all characters & numbers
 * are all the same width.
 *
 * This font also contains a few special characters that are nice for certain applications
 * like clocks, signed values or decimal point values.
 * 
 * The rendering code normally inserts a pad pixel so this size allows the font to fit
 * perfectly on 32 and 64 tall glcd displays.
 *
 * Font has also been squeezed to 15 pixels wide for better alignment on 128 & 192 pixel displays.
 */

#ifndef FIXEDNUMS15x31_H
#define FIXEDNUMS15x31_H

#include <inttypes.h>


static char fixednums15x31[] = {
    0x0, 0x0,	// size of zero indicates fixed width font
    15,		// width
    31,		// height
    '+',	// first char (48)
    16,		// char count
// char '+'
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xfc, 0xfc, 0xfc, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

// char ','
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf, 0xcf, 0x3f, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00,

// char '-'
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

// char '.'
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,

// char '/'
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xff, 0xff, 0x0f,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xff, 0xff, 0x0f, 0x0f, 0x00, 0x00, 0x00,
0x00, 0x00, 0xf0, 0xf0, 0xff, 0xff, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

// char '0'
0xf0, 0xf0, 0xfc, 0xfc, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0xcf, 0xcf, 0xfc, 0xfc, 0xf0, 0xf0,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xc0, 0xc0, 0x3c, 0x3f, 0x03, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0x3c, 0x3c, 0x03, 0x03, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x03, 0x03, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00, 0x00,

// char '1'
0x30, 0x30, 0x30, 0x30, 0x3c, 0x3c, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,

// char '2'
0xf0, 0xf0, 0xfc, 0xfc, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0x03, 0x0f, 0xff, 0xfc, 0xfc, 0xf0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xf0, 0xff, 0x3f, 0x3f, 0x0f,
0xc0, 0xc0, 0xf0, 0xf0, 0x3c, 0x3c, 0x0f, 0x0f, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
0x0f, 0x0f, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,

// char '3'
0xf0, 0xf0, 0xfc, 0xfc, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0x03, 0x0f, 0x1f, 0xfc, 0xfc, 0xf0,
0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0xff, 0xdf, 0x8f,
0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff,
0x00, 0x00, 0x03, 0x03, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00,

// char '4'
0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xff, 0xff, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0xfc, 0xfc, 0xff, 0xff, 0x03, 0x03, 0xc0, 0xc0, 0xc0, 0xc0, 0x00, 0x00, 0x00,
0xff, 0xff, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xc0, 0xc0,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00,

// char '5'
0xff, 0xff, 0xff, 0xff, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
0x0f, 0x0f, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0xfc, 0xf0, 0xf0, 0xc0,
0xf0, 0xf0, 0xf0, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x03, 0x03, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00,

// char '6'
0x00, 0x00, 0xc0, 0xc0, 0xf0, 0xf0, 0x3c, 0x3c, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0x03, 0x03,
0xfc, 0xfc, 0xff, 0xff, 0x33, 0x33, 0x30, 0x30, 0x30, 0x30, 0xf0, 0xf0, 0xc0, 0xc0, 0x00,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x03, 0x03, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00,

// char '7'
0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xff, 0xff, 0x3f, 0x3f, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xf0, 0xff, 0xff, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

// char '8'
0xf0, 0xf0, 0xfc, 0xfc, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0x0f, 0x0f, 0xfc, 0xfc, 0xf0, 0xf0,
0x03, 0x03, 0xcf, 0xcf, 0xfc, 0xfc, 0x30, 0x30, 0x30, 0xfc, 0xfc, 0xcf, 0xcf, 0x03, 0x03,
0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x03, 0x03, 0x0f, 0x0f, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00, 0x00,

// char '9'
0xe0, 0xf0, 0xfc, 0xfc, 0x0f, 0x0f, 0x03, 0x03, 0x03, 0x0f, 0x0f, 0xfc, 0xfc, 0xf0, 0xe0,
0x07, 0x0f, 0x3f, 0x3f, 0xf0, 0xf0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xc0, 0xfc, 0xfc, 0x3f, 0x3f, 0x03,
0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0f, 0x0f, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,

// char ':'
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,

};

#endif
