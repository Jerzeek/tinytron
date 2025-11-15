#pragma once
#include <Arduino.h>
#include "JPEGDEC.h"

#include "VideoPlayerState.h"
#include "StreamVideoSource.h"

#include "Battery.h"

class Display;
class Prefs;
class VideoSource;

class VideoPlayer {
  private:
    int mChannelVisible = 0;
    VideoPlayerState mState = VideoPlayerState::STOPPED;

    // video playing
    Display &mDisplay;
    JPEGDEC mJpeg;
    Prefs &mPrefs;
    Battery &mBattery;

    // video source
    VideoSource *mVideoSource = NULL;

    TaskHandle_t _framePlayerTaskHandle = NULL;
    volatile bool m_runTask = false;

    static void _framePlayerTask(void *param);

    void framePlayerTask();
    void playTask();
    void drawOSD(int fps);

    friend int _doDraw(JPEGDRAW *pDraw);

  public:
    VideoPlayer(VideoSource *videoSource, Display &display, Prefs &prefs, Battery &battery);
    void setChannel(int channelIndex);
    void nextChannel();
    void start();
    void play();
    void stop();
    void pause();
    void playStatic();
    void playPauseToggle();
};