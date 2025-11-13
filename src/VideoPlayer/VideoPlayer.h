#pragma once
#include <Arduino.h>
#include "JPEGDEC.h"

#include "VideoPlayerState.h"

class Display;

class VideoSource;

class VideoPlayer {
  private:
    int mChannelVisible = 0;
    VideoPlayerState mState = VideoPlayerState::STOPPED;

    // video playing
    Display &mDisplay;
    JPEGDEC mJpeg;

    // video source
    VideoSource *mVideoSource = NULL;

    TaskHandle_t _framePlayerTaskHandle = NULL;
    volatile bool m_runTask = false;

    static void _framePlayerTask(void *param);

    void framePlayerTask();
    void playTask();

    friend int _doDraw(JPEGDRAW *pDraw);

  public:
    VideoPlayer(VideoSource *videoSource, Display &display);
    void setChannel(int channelIndex);
    void nextChannel();
    void start();
    void play();
    void stop();
    void pause();
    void playStatic();
    void playPauseToggle();
};