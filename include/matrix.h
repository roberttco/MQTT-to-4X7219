#include <MD_MAX72xx.h>
#include <SPI.h>

#ifndef MATRIX_H
#define MATRIX_H

#define MAX_DEVICES 4

#define CLK_PIN D5  // or SCK
#define DATA_PIN D7 // or MOSI
#define CS_PIN D8   // or SS

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

// const uint8_t CHAR_SPACING = 1;
// uint8_t SCROLL_DELAY = 75;
// uint8_t DEFAULT_INTENSITY = 5;
// uint8_t DIM_INTENSITY = 2;

#define CHAR_SPACING 1
#define SCROLL_DELAY 75
#define DEFAULT_INTENSITY 5
#define DIM_INTENSITY 2

void printText(MD_MAX72XX * mx, uint8_t modStart, uint8_t modEnd, const char *pMsg);

#endif // MATRIX_H