#ifndef NJU6676_H
#define NJU6676_H

#include <stdint.h>
// #include "bcm2835.h"
// #include "PiRadio.h"
#include "utilities.h"

// Screen dimensions [0 127 0 63]
#define SCREEN_WIDTH     128  // 3 scratch + 128 
#define SCREEN_HEIGHT     64  // 1 icon + 64

// Colors
#define BLUE            0x00
#define WHITE           0xFF

// Font Indices
#define FONT_LENGTH        0
#define FONT_FIXED_WIDTH   2
#define FONT_HEIGHT        3
#define FONT_FIRST_CHAR    4
#define FONT_CHAR_COUNT    5
#define FONT_WIDTH_TABLE   6

typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t page;
} lcdCoord;

typedef uint8_t (*nju6676FontCallback)(const char*);
extern uint8_t DDRAM[1024];

// #define nju6676ClearScreen() {nju6676FillRect(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLUE);}

// Hardware functions
void nju6676Init(void);
void nju6676ResetHW( void );
void nju6676Backlight( int State );

// Graphical functions
void nju6676DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void nju6676DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void nju6676DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);
void nju6676FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void nju6676SetPixels(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, uint8_t color);
void nju6676InvertRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void nju6676SetInverted(uint8_t invert);
void nju6676SetDot(uint8_t x, uint8_t y, uint8_t color);
void nju6676ClearScreen(uint8_t color);
void nju6676DrawBitmap(uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t w, uint8_t h, uint8_t color);
void nju6676DrawHLine(uint8_t x, uint8_t y, uint8_t width, uint8_t color);
void nju6676DrawVLine(uint8_t x, uint8_t y, uint8_t height, uint8_t color);
void nju6676DrawCircle(uint8_t xCenter, uint8_t yCenter, uint8_t radius, uint8_t color);
void nju6676FillCircle(uint8_t xCenter, uint8_t yCenter, uint8_t radius, uint8_t color);

// Font functions
uint8_t nju6676ReadFontData(const char* ptr);		//Standard Read Callback
void nju6676SelectFont(const char* font, nju6676FontCallback callback, uint8_t color);
int nju6676PutChar(char c);
void nju6676Puts(char* str);
uint8_t nju6676CharWidth(char c);
uint16_t nju6676StringWidth(char* str);
void nju6676PrintNumber(long n);

// Control functions
void nju6676GotoXY(uint8_t x, uint8_t y);
inline uint8_t nju6676ReadData(void);
void nju6676WriteCommand( uint8_t data);
void nju6676WriteData( uint8_t data);
void nju6676DoWriteData( uint8_t data);
void nju6676Display(void);

#endif
