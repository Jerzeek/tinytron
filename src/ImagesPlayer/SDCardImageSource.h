#pragma once

#include "../VideoPlayer/VideoSource.h"
#include <string>
#include <vector>

class SDCard;

class SDCardImageSource : public VideoSource {
private:
  std::vector<std::string> mImageFiles;
  SDCard *mSDCard;
  const char *mPath;
  bool mShowFilename;
  unsigned long mLastChangeTime = 0;
  unsigned long mIntervalMs = 5000;
  bool mForceNext = true;

  bool loadCurrentImage(uint8_t **buffer, size_t &bufferLength,
                        size_t &frameLength);

public:
  SDCardImageSource(SDCard *sdCard, const char *path, bool showFilename = true);
  void start() override;
  bool fetchChannelData() override;
  bool getVideoFrame(uint8_t **buffer, size_t &bufferLength,
                     size_t &frameLength) override;
  void setChannel(int channel) override;
  void nextChannel() override;
  int getChannelCount() override { return mImageFiles.size(); }
  std::string getChannelName() override;
  bool isStillImageSource() override { return true; }
  bool showChannelNameOSD() override { return mShowFilename; }
  uint32_t getAutoAdvanceIntervalMs() override { return (uint32_t)mIntervalMs; }
};
