#ifndef UTILITIES_H 
#define UTILITIES_H 
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "bcm2835.h"


#define LCD_A0      RPI_GPIO_P1_07
#define LCD_RST     RPI_GPIO_P1_08
#define LED_ON_OFF  RPI_GPIO_P1_10
#define POWER_EN    RPI_GPIO_P1_11
#define I2C_RST     RPI_GPIO_P1_12
#define ON  1
#define OFF 0

void GPIO_Setup( void );
void LED_state( int State );
int SPI_Setup( void );
void SPI_Finish( void );
pid_t proc_find(const char* name);

#endif
