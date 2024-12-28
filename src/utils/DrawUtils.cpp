#include <utils/DrawUtils.h>
#include <utils/Colors.h>
#include <savemng.h>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <malloc.h>
#include <tga_reader.h>
#include <coreinit/debug.h>
#include <coreinit/cache.h>
#include <coreinit/screen.h>
#include <coreinit/memheap.h>
#include <coreinit/memfrmheap.h>
#include <coreinit/memory.h>
#include <proc_ui/procui.h>

#include <whb/log_udp.h>
#include <whb/log.h>

// buffer width
#define TV_WIDTH  0x500
#define DRC_WIDTH 0x380

#define CONSOLE_FRAME_HEAP_TAG (0x000DECAF)


bool DrawUtils::isBackBuffer;

uint8_t *DrawUtils::tvBuffer = nullptr;
uint8_t *DrawUtils::drcBuffer = nullptr;
uint32_t DrawUtils::sBufferSizeTV = 0, DrawUtils::sBufferSizeDRC = 0;
BOOL DrawUtils::sConsoleHasForeground = TRUE;

static SFT cFont = {};
static SFT kFont = {};
static SFT pFont = {};

static Color font_col(0xFFFFFFFF);

uint32_t
DrawUtils::initScreen()
{
   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   MEMRecordStateForFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);

   if (sBufferSizeTV) {
      DrawUtils::tvBuffer = static_cast<uint8_t *>(MEMAllocFromFrmHeapEx(heap, sBufferSizeTV, 4));
   }

   if (sBufferSizeDRC) {
      DrawUtils::drcBuffer = static_cast<uint8_t *>(MEMAllocFromFrmHeapEx(heap, sBufferSizeDRC, 4));
   }

    
   sConsoleHasForeground = TRUE;

   OSScreenSetBufferEx(SCREEN_TV, DrawUtils::tvBuffer);
   OSScreenSetBufferEx(SCREEN_DRC, DrawUtils::drcBuffer);

   DrawUtils::endDraw(); // flip buffers

   DrawUtils::setRedraw(true); // force a redraw when reentering

   return 0;

}

uint32_t
DrawUtils::deinitScreen()
{

   MEMHeapHandle heap = MEMGetBaseHeapHandle(MEM_BASE_HEAP_MEM1);
   MEMFreeByStateToFrmHeap(heap, CONSOLE_FRAME_HEAP_TAG);
   sConsoleHasForeground = FALSE;
   return 0;

}


BOOL
DrawUtils::LogConsoleInit()
{
   OSScreenInit();
   sBufferSizeTV = OSScreenGetBufferSizeEx(SCREEN_TV);
   sBufferSizeDRC = OSScreenGetBufferSizeEx(SCREEN_DRC);

   initScreen();

   OSScreenEnableEx(SCREEN_TV, 1);
   OSScreenEnableEx(SCREEN_DRC, 1);

   return FALSE;
}

void
DrawUtils::LogConsoleFree()
{
   if (sConsoleHasForeground) {
      deinitScreen();
      OSScreenShutdown();
   }
}


bool DrawUtils::redraw = true;

template<typename T>
inline typename std::unique_ptr<T> make_unique_nothrow(size_t num) noexcept {
    return std::unique_ptr<T>(new (std::nothrow) std::remove_extent_t<T>[num]());
}

void DrawUtils::setRedraw(bool value) {
    redraw = value;
}

void DrawUtils::beginDraw() {
    uint32_t pixel = *(uint32_t *) tvBuffer;

    // check which buffer is currently used
    OSScreenPutPixelEx(SCREEN_TV, 0, 0, 0xABCDEF90);
    if (*(uint32_t *) tvBuffer == 0xABCDEF90) {
        isBackBuffer = false;
    } else {
        isBackBuffer = true;
    }

    // restore the pixel we used for checking
    *(uint32_t *) tvBuffer = pixel;
}

void DrawUtils::endDraw() {
    OSScreenFlipBuffersEx(SCREEN_DRC);
    OSScreenFlipBuffersEx(SCREEN_TV);
}

void DrawUtils::clear(Color col) {
    OSScreenClearBufferEx(SCREEN_TV, col.color);
    OSScreenClearBufferEx(SCREEN_DRC, col.color);
}

void DrawUtils::drawPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    float opacity = a / 255.0f;

    // put pixel in the drc buffer
    uint32_t i = (x + y * DRC_WIDTH) * 4;
    if (i + 3 < sBufferSizeDRC / 2) {
        if (isBackBuffer) {
            i += sBufferSizeDRC / 2;
        }
        if (a == 0xFF) {
            drcBuffer[i] = r;
            drcBuffer[i + 1] = g;
            drcBuffer[i + 2] = b;
        } else {
            drcBuffer[i] = r * opacity + drcBuffer[i] * (1 - opacity);
            drcBuffer[i + 1] = g * opacity + drcBuffer[i + 1] * (1 - opacity);
            drcBuffer[i + 2] = b * opacity + drcBuffer[i + 2] * (1 - opacity);
        }
    }

    uint32_t USED_TV_WIDTH = TV_WIDTH;
    float scale = 1.5f;
    if (DrawUtils::sBufferSizeTV == 0x00FD2000) {
        USED_TV_WIDTH = 1920;
        scale = 2.25f;
    }

    // scale and put pixel in the tv buffer
    for (uint32_t yy = (y * scale); yy < ((y * scale) + (uint32_t) scale); yy++) {
        for (uint32_t xx = (x * scale); xx < ((x * scale) + (uint32_t) scale); xx++) {
            uint32_t i = (xx + yy * USED_TV_WIDTH) * 4;
            if (i + 3 < sBufferSizeTV / 2) {
                if (isBackBuffer) {
                    i += sBufferSizeTV / 2;
                }
                if (a == 0xFF) {
                    tvBuffer[i] = r;
                    tvBuffer[i + 1] = g;
                    tvBuffer[i + 2] = b;
                } else {
                    tvBuffer[i] = r * opacity + tvBuffer[i] * (1 - opacity);
                    tvBuffer[i + 1] = g * opacity + tvBuffer[i + 1] * (1 - opacity);
                    tvBuffer[i + 2] = b * opacity + tvBuffer[i + 2] * (1 - opacity);
                }
            }
        }
    }
}

void DrawUtils::drawRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    drawLine(x1, y1, x2, y1, r, g, b, a);
    drawLine(x2, y1, x2, y2, r, g, b, a);
    drawLine(x1, y2, x2, y2, r, g, b, a);
    drawLine(x1, y1, x1, y2, r, g, b, a);
}


bool DrawUtils::initFont(OSSharedDataType fontType) {
    void *font = nullptr;
    uint32_t size = 0;
    OSGetSharedData(fontType, 0, &font, &size);

    if (font && size) {
        pFont.xScale = 22;
        pFont.yScale = 22,
        pFont.flags = SFT_DOWNWARD_Y;
        pFont.font = sft_loadmem(font, size);
        if (!pFont.font) {
            return false;
        }
        OSMemoryBarrier();
        return true;
    }
    return false;
}


bool DrawUtils::initKFont() {
    uint8_t * kfont = nullptr;
    uint32_t size = 0;
    size =  loadFile("romfs:/ttf/21513_Keyboard.ttf",&kfont);
    if (kfont && size) {
        kFont.xScale = 44;
        kFont.yScale = 44,
        kFont.flags = SFT_DOWNWARD_Y;
        kFont.font = sft_loadmem(kfont, size);
        if (!kFont.font) {
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool DrawUtils::setFont() {
    cFont = pFont;
    return true;
}
bool DrawUtils::setKFont() {
    cFont = kFont;
    return true;
}

void DrawUtils::deinitFont() {
    sft_freefont(pFont.font);
    pFont.font = nullptr;
    pFont = {};

    sft_freefont(kFont.font);
    kFont.font = nullptr;
    kFont = {};
}

void DrawUtils::setFontColor(Color col) {
    font_col = col;
}

void DrawUtils::setFontColorByCursor(Color col, Color colAtCursor,int cursorPos, int line)  {
    if (cursorPos == line)
        font_col = colAtCursor;
    else
        font_col = col;
}

static void draw_freetype_bitmap(SFT_Image *bmp, int32_t x, int32_t y) {
    int32_t i, j, p, q;

    int32_t x_max = x + bmp->width;
    int32_t y_max = y + bmp->height;

    auto *src = (uint8_t *) bmp->pixels;

    for (i = x, p = 0; i < x_max; i++, p++) {
        for (j = y, q = 0; j < y_max; j++, q++) {
            if (i < 0 || j < 0 || i >= SCREEN_WIDTH || j >= SCREEN_HEIGHT) {
                continue;
            }

            float opacity = src[q * bmp->width + p] / 255.0f;
            DrawUtils::drawPixel(i, j, font_col.r, font_col.g, font_col.b, font_col.a * opacity);
        }
    }
}

void DrawUtils::print(uint32_t x, uint32_t y, const char *string, bool alignRight) {
    auto *buffer = new wchar_t[strlen(string) + 1];

    size_t num = mbstowcs(buffer, string, strlen(string));
    if (num > 0) {
        buffer[num] = 0;
    } else {
        wchar_t *tmp = buffer;
        while ((*tmp++ = *string++))
            ;
    }

    print(x, y, buffer, alignRight);
    delete[] buffer;
}

void DrawUtils::print(uint32_t x, uint32_t y, const wchar_t *string, bool alignRight) {
    auto penX = (int32_t) x;
    auto penY = (int32_t) y;

    if (alignRight) {
        penX -= getTextWidth(string);
    }

    uint16_t textureWidth = 0, textureHeight = 0;
    for (; *string; string++) {
        SFT_Glyph gid; //  unsigned long gid;
        if (sft_lookup(&cFont, *string, &gid) >= 0) {
            SFT_GMetrics mtx;
            if (sft_gmetrics(&cFont, gid, &mtx) < 0) {
                return;
            }



            if (*string == '\n') {
                //penY += mtx.minHeight;
                penY += 30; // temporal fix for multiline output 
                penX = x;
                continue;
            }

            

            textureWidth = (mtx.minWidth + 3) & ~3;
            textureHeight = mtx.minHeight;

            SFT_Image img = {
                    .pixels = nullptr,
                    .width = textureWidth,
                    .height = textureHeight,
            };

            if (textureWidth == 0) {
                textureWidth = 4;
            }
            if (textureHeight == 0) {
                textureHeight = 4;
            }

            auto buffer = make_unique_nothrow<uint8_t[]>((uint32_t) (img.width * img.height));
            if (!buffer) {
                return;
            }
            img.pixels = buffer.get();
            if (sft_render(&cFont, gid, img) < 0) {
                return;
            } else {
                draw_freetype_bitmap(&img, (int32_t) (penX + mtx.leftSideBearing), (int32_t) (penY + mtx.yOffset));
                penX += (int32_t) mtx.advanceWidth;
            }
        }
    }
}

uint32_t DrawUtils::getTextWidth(const char *string) {
    auto *buffer = new wchar_t[strlen(string) + 1];

    size_t num = mbstowcs(buffer, string, strlen(string));
    if (num > 0) {
        buffer[num] = 0;
    } else {
        wchar_t *tmp = buffer;
        while ((*tmp++ = *string++))
            ;
    }

    uint32_t width = getTextWidth(buffer);
    delete[] buffer;

    return width;
}

uint32_t DrawUtils::getTextWidth(const wchar_t *string) {
    uint32_t width = 0;

    for (; *string; string++) {
        SFT_Glyph gid; //  unsigned long gid;
        if (sft_lookup(&cFont, *string, &gid) >= 0) {
            SFT_GMetrics mtx;
            sft_gmetrics(&cFont, gid, &mtx);
            width += (int32_t) mtx.advanceWidth;
        }
    }

    return (uint32_t) width;
}

void DrawUtils::drawLine(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int x;
    int y;
    if (x1 == x2) {
        if (y1 < y2)
            for (y = y1; y <= y2; y++)
                DrawUtils::drawPixel(x1, y, r, g, b, a);
        else
            for (y = y2; y <= y1; y++)
                DrawUtils::drawPixel(x1, y, r, g, b, a);
    } else {
        if (x1 < x2)
            for (x = x1; x <= x2; x++)
                DrawUtils::drawPixel(x, y1, r, g, b, a);
        else
            for (x = x2; x <= x1; x++)
                DrawUtils::drawPixel(x, y1, r, g, b, a);
    }
}

void DrawUtils::drawPic(int x, int y, uint32_t w, uint32_t h, float scale, const uint32_t *pixels) {
    uint32_t nw = w, nh = h;
    Color color(0xFFFFFFFF);
    if (scale <= 0) scale = 1;
    nw = w * scale;
    nh = h * scale;


    for (uint32_t j = y; j < (y + nh); j++) {
        for (uint32_t i = x; i < (x + nw); i++) {
            color.color = pixels[((i - x) * w / nw) + ((j - y) * h / nh) * w];
            DrawUtils::drawPixel(i, j, color);
        }
    }
}

void DrawUtils::drawTGA(int x, int y, float scale, uint8_t *fileContent) {
    uint32_t w = tgaGetWidth(fileContent), h = tgaGetHeight(fileContent);
    uint32_t nw = w, nh = h;
    auto *out = (uint32_t *) tgaRead(fileContent, TGA_READER_RGBA);

    if (scale <= 0) scale = 1;
    nw = w * scale;
    nh = h * scale;

    drawPic(x, y, w, h, scale, out);
    if (scale > 0.5) {
        if ((x > 0) && (y > 0)) drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
        if ((x > 1) && (y > 1)) drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
        if ((x > 2) && (y > 2)) drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
    }
    free(out);
}

void DrawUtils::drawRGB5A3(int x, int y, float scale, uint8_t *fileContent) {
    uint32_t w = 192;
    uint32_t h = 64;
    uint32_t nw = w;
    uint32_t nh = h;

    if (scale <= 0)
        scale = 1;
    nw = w * scale;
    nh = h * scale;

    uint32_t pos = 0;
    uint32_t npos = 0;
    Color color(0xFFFFFFFF);
    auto *pixels = (uint16_t *) fileContent;

    uint8_t sum = (4 * scale);
    for (uint32_t j = y; j < (y + nh); j += sum) {
        for (uint32_t i = x; i < (x + nw); i += sum) {
            for (uint32_t sj = j; sj < (j + sum); sj++, pos++) {
                for (uint32_t si = i; si < (i + sum); si++) {
                    npos = ((si - i) / scale) + ((pos / scale) * 4);
                    if ((pixels[npos] & 0x8000) != 0) {
                        color.color = ((pixels[npos] & 0x7C00) << 17) | ((pixels[npos] & 0x3E0) << 14) |
                                      ((pixels[npos] & 0x1F) << 11) | 0xFF;
                    } else {
                        color.color = (((pixels[npos] & 0xF00) * 0x11) << 16) | (((pixels[npos] & 0xF0) * 0x11) << 12) |
                                      (((pixels[npos] & 0xF) * 0x11) << 8) | (((pixels[npos] & 0x7000) >> 12) * 0x24);
                    }
                    DrawUtils::drawPixel(si, sj, color);
                }
            }
        }
    }
    if (scale > 0.5) {
        if ((x > 0) && (y > 0)) {
            drawRect(x - 1, y - 1, x + nw, y + nh, 255, 255, 255, 128);
        }
        if ((x > 1) && (y > 1)) {
            drawRect(x - 2, y - 2, x + nw + 1, y + nh + 1, 255, 255, 255, 128);
        }
        if ((x > 2) && (y > 2)) {
            drawRect(x - 3, y - 3, x + nw + 2, y + nh + 2, 255, 255, 255, 128);
        }
    }
}

void DrawUtils::drawKey(int x,int y,int x_off,Color color) {
    int xtop = x*52-26+x_off+5;
    int ytop = (y+Y_OFF)*50-25-5;
    drawRect(xtop,ytop,xtop+52,ytop+50,color.r,color.g,color.b,color.a);

}