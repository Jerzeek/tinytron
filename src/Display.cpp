#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Display.h"

// PWM channel for backlight
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_8_BIT 8
#define LEDC_BASE_FREQ 5000

Display::Display(Prefs *prefs) : tft(new TFT_eSPI()), _prefs(prefs)
{
  tft_mutex = xSemaphoreCreateRecursiveMutex();

  // First, initialize the TFT itself and set the rotation
  tft->init();
  tft->setRotation(3);

  // Now create the sprite with the correct, rotated dimensions
  frameSprite = new TFT_eSprite(tft);
  
  // Try to create the sprite. If it fails (returns nullptr), we fall back to direct drawing.
  if (frameSprite->createSprite(tft->width(), tft->height()) == nullptr) {
    Serial.println("Warning: Failed to create full-screen sprite. Falling back to direct draw.");
    delete frameSprite;
    frameSprite = nullptr;
  } else {
    frameSprite->setTextFont(2);
    frameSprite->setTextSize(1);
  }

// setup the backlight
#ifdef TFT_BL
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_8_BIT);
  ledcAttachPin(TFT_BL, LEDC_CHANNEL_0);
  ledcWrite(LEDC_CHANNEL_0, 255); // turn on backlight
#endif

  if (frameSprite) {
    tft->fillScreen(TFT_BLACK); // Clear real screen once
  } else {
    tft->fillScreen(TFT_BLACK);
  }

#ifdef USE_DMA
  tft->initDMA();
#endif

  if (frameSprite) {
    // Sprite mode specific setup if needed
  } else {
    tft->setTextFont(2);
    tft->setTextSize(1);
    tft->setTextColor(TFT_GREEN, TFT_BLACK);
  }
}

void Display::setBrightness(uint8_t brightness)
{
#ifdef TFT_BL
  ledcWrite(LEDC_CHANNEL_0, brightness);
#endif
}

// this function now draws directly to the screen, used for non-buffered drawing
void Display::drawPixels(int x, int y, int width, int height, uint16_t *pixels)
{
  int numPixels = width * height;
  if (dmaBuffer[dmaBufferIndex] == NULL)
  {
    // Try to allocate. If fails, we might crash or need to handle it.
    // Given the constraints, we hope this fits if called. 
    // But drawPixelsToSprite is the main one used.
    dmaBuffer[dmaBufferIndex] = (uint16_t *)malloc(numPixels * 2);
  }
  
  if (dmaBuffer[dmaBufferIndex] != NULL) {
    memcpy(dmaBuffer[dmaBufferIndex], pixels, numPixels * 2);
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    tft->setAddrWindow(x, y, width, height);
  #ifdef USE_DMA
    tft->pushPixelsDMA(dmaBuffer[dmaBufferIndex], numPixels);
  #else
    tft->pushPixels(dmaBuffer[dmaBufferIndex], numPixels);
  #endif
    xSemaphoreGiveRecursive(tft_mutex);
    dmaBufferIndex = (dmaBufferIndex + 1) % 2;
  } else {
    // Fallback if malloc failed: synchronous slow draw
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    tft->pushImage(x, y, width, height, pixels);
    xSemaphoreGiveRecursive(tft_mutex);
  }
}

// new function to draw to our framebuffer sprite
void Display::drawPixelsToSprite(int x, int y, int width, int height, uint16_t *pixels)
{
  if (frameSprite) {
    frameSprite->pushImage(x, y, width, height, pixels);
  } else {
    // Direct draw fallback
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    tft->pushImage(x, y, width, height, pixels);
    xSemaphoreGiveRecursive(tft_mutex);
  }
}

// new function to push the framebuffer to the screen
void Display::flushSprite()
{
  if (frameSprite) {
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    frameSprite->pushSprite(0, 0);
    xSemaphoreGiveRecursive(tft_mutex);
  }
  // If no sprite, we drew directly, so nothing to flush.
}

void Display::fillSprite(uint16_t color)
{
  if (frameSprite) {
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    frameSprite->fillSprite(color);
    xSemaphoreGiveRecursive(tft_mutex);
  } else {
    xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
    tft->fillScreen(color);
    xSemaphoreGiveRecursive(tft_mutex);
  }
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
  if (frameSprite) {
    frameSprite->fillSprite(color);
  } else {
    tft->fillScreen(color);
  }
}

void Display::drawOSD(const char *text, OSDPosition position, OSDLevel level)
{
  if (_prefs->getOsdLevel() < level)
  {
    return;
  }
  xSemaphoreTakeRecursive(tft_mutex, portMAX_DELAY);
  
  // Decide where to draw
  TFT_eSPI* target = frameSprite ? (TFT_eSPI*)frameSprite : tft;
  
  // Set up font/colors
  target->setTextColor(TFT_ORANGE, TFT_BLACK);
  // Re-ensure font/size if using direct tft as it might have changed? 
  // Good practice to set it before use.
  target->setTextFont(2);
  target->setTextSize(1);

  int textWidth = target->textWidth(text);
  int textHeight = target->fontHeight();
  int x = 0;
  int y = 0;

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
  target->setCursor(x, y);
  target->println(text);
  xSemaphoreGiveRecursive(tft_mutex);
}