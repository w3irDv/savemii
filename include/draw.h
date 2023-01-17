#pragma once

#include <coreinit/screen.h>
#include <tga_reader.h>

void drawRectFilled(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void drawTGA(int x, int y, float scale, uint8_t *fileContent);
void drawRGB5A3(int x, int y, float scale, uint8_t *pixels);