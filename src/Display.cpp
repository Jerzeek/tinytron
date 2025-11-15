#ifndef LED_MATRIX
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Display.h"

// PWM channel for backlight
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_8_BIT 8
#define LEDC_BASE_FREQ 5000

Display::Display() : tft(new TFT_eSPI())
{
// setup the backlight
#ifdef TFT_BL
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT);
  ledcAttachPin(TFT_BL, LEDC_CHANNEL_0);
  ledcWrite(LEDC_CHANNEL_0, 255); // turn on backlight
#endif

  tft->init();
  tft->setRotation(3);
  tft->fillScreen(TFT_BLACK);
#ifdef USE_DMA
  tft->initDMA();
#endif
  tft->fillScreen(TFT_BLACK);
  tft->setTextFont(2);
  tft->setTextSize(2);
  tft->setTextColor(TFT_GREEN, TFT_BLACK);
}

void Display::setBrightness(uint8_t brightness)
{
#ifdef TFT_BL
  ledcWrite(LEDC_CHANNEL_0, brightness);
#endif
}

void Display::drawPixels(int x, int y, int width, int height, uint16_t *pixels)
{
  int numPixels = width * height;
  if (dmaBuffer[dmaBufferIndex] == NULL)
  {
    dmaBuffer[dmaBufferIndex] = (uint16_t *)malloc(numPixels * 2);
  }
  memcpy(dmaBuffer[dmaBufferIndex], pixels, numPixels * 2);
#ifdef USE_DMA
  tft->dmaWait();
#endif
  tft->setAddrWindow(x, y, width, height);
#ifdef USE_DMA
  tft->pushPixelsDMA(dmaBuffer[dmaBufferIndex], numPixels);
#else
  tft->pushPixels(dmaBuffer[dmaBufferIndex], numPixels);
#endif
  dmaBufferIndex = (dmaBufferIndex + 1) % 2;
}

void Display::startWrite()
{
  tft->startWrite();
}

void Display::endWrite()
{
  tft->endWrite();
}

int Display::width()
{
  return tft->width();
}

int Display::height()
{
  return tft->height();
}

void Display::fillScreen(uint16_t color)
{
  tft->fillScreen(color);
}

void Display::drawOSD(const char *text, OSDPosition position)
{
  tft->setTextColor(TFT_GREEN, TFT_BLACK);
  int x = 0;
  int y = 0;
  int textWidth = tft->textWidth(text);
  int textHeight = tft->fontHeight();
  switch (position)
  {
  case TOP_LEFT:
    x = 20;
    y = 20;
    break;
  case TOP_RIGHT:
    x = width() - textWidth - 20;
    y = 20;
    break;
  case BOTTOM_LEFT:
    x = 20;
    y = height() - textHeight - 20;
    break;
  case BOTTOM_RIGHT:
    x = width() - textWidth - 20;
    y = height() - textHeight - 20;
    break;
  case CENTER:
    x = (width() - textWidth) / 2;
    y = (height() - textHeight) / 2;
    break;
  }
  tft->setCursor(x, y);
  tft->println(text);
}

void Display::drawSDCardFailed()
{
  tft->fillScreen(TFT_RED);
  tft->setCursor(0, 20);
  tft->setTextColor(TFT_WHITE);
  tft->setTextSize(2);
  tft->println("Failed to mount SD Card");
}
#endif