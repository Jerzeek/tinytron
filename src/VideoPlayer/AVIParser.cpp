#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AVIParser.h"

typedef struct
{
  char chunkId[4];
  unsigned int chunkSize;
} ChunkHeader;

void readChunk(FILE *file, ChunkHeader *header)
{
  fread(&header->chunkId, 4, 1, file);
  fread(&header->chunkSize, 4, 1, file);
  // Serial.printf("ChunkId %c%c%c%c, size %u\n",
  //        header->chunkId[0], header->chunkId[1],
  //        header->chunkId[2], header->chunkId[3],
  //        header->chunkSize);
}

AVIParser::AVIParser(std::string fname, AVIChunkType requiredChunkType): mFileName(fname), mRequiredChunkType(requiredChunkType)
{
}

AVIParser::~AVIParser()
{
  if (mFile)
  {
    fclose(mFile);
  }
}

// http://www.fastgraph.com/help/avi_header_format.html
typedef struct
{
  unsigned int dwMicroSecPerFrame;
  unsigned int dwMaxBytesPerSec;
  unsigned int dwPaddingGranularity;
  unsigned int dwFlags;
  unsigned int dwTotalFrames;
  unsigned int dwInitialFrames;
  unsigned int dwStreams;
  unsigned int dwSuggestedBufferSize;
  unsigned int dwWidth;
  unsigned int dwHeight;
  unsigned int dwScale;
  unsigned int dwRate;
  unsigned int dwStart;
  unsigned int dwLength;
} MainAVIHeader;


bool AVIParser::open()
{
  mFile = fopen(mFileName.c_str(), "rb");
  if (!mFile)
  {
    Serial.printf("Failed to open file.\n");
    return false;
  }
  // check the file is valid
  ChunkHeader header;
  // Read RIFF header
  readChunk(mFile, &header);
  if (strncmp(header.chunkId, "RIFF", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    fclose(mFile);
    mFile = NULL;
    return false;
  }
  // next four bytes are the RIFF type which should be 'AVI '
  char riffType[4];
  fread(&riffType, 4, 1, mFile);
  if (strncmp(riffType, "AVI ", 4) != 0)
  {
    Serial.println("Not a valid AVI file.");
    fclose(mFile);
    mFile = NULL;
    return false;
  }

  // now read each chunk and find the movi list
  while (!feof(mFile) && !ferror(mFile))
  {
    readChunk(mFile, &header);
    if (feof(mFile) || ferror(mFile))
    {
      break;
    }
    // is it a LIST chunk?
    if (strncmp(header.chunkId, "LIST", 4) == 0)
    {
      long listContentPosition = ftell(mFile);
      char listType[4];
      fread(&listType, 4, 1, mFile);

      if (strncmp(listType, "hdrl", 4) == 0)
      {
        // This is the header list, which contains the 'avih' chunk with the frame rate
        ChunkHeader avihHeader;
        readChunk(mFile, &avihHeader);
        if (strncmp(avihHeader.chunkId, "avih", 4) == 0)
        {
          MainAVIHeader avih;
          fread(&avih, sizeof(MainAVIHeader), 1, mFile);
          mFrameRate = (float)avih.dwRate / avih.dwScale;
          Serial.printf("Frame rate: %f\n", mFrameRate);
        }
        // We've processed what we need from the 'hdrl' list, so skip to the end of it.
        fseek(mFile, listContentPosition + header.chunkSize, SEEK_SET);
      }
      else if (strncmp(listType, "movi", 4) == 0)
      {
        // This is the movie list. We've found what we're looking for.
        Serial.printf("Found movi list.\n");
        mMoviListPosition = ftell(mFile); // The current position is the start of the movi data
        mMoviListLength = header.chunkSize - 4;
        Serial.printf("List Chunk Length: %ld\n", mMoviListLength);
        // We can stop parsing the file now.
        break;
      }
      else
      {
        // This is some other kind of LIST chunk that we don't care about. Skip it.
        fseek(mFile, header.chunkSize - 4, SEEK_CUR);
      }
    }
    else
    {
      // This is not a LIST chunk. Skip it.
      fseek(mFile, header.chunkSize, SEEK_CUR);
    }
  }

  if (mMoviListPosition == 0)
  {
    Serial.printf("Failed to find the movi list.\n");
    fclose(mFile);
    mFile = NULL;
    return false;
  }

  // Before we return, we must position the file pointer at the start of the movi data.
  fseek(mFile, mMoviListPosition, SEEK_SET);
  return true;
}

size_t AVIParser::getNextChunk(uint8_t **buffer, size_t &bufferLength)
{
  // check if the file is open
  if (!mFile)
  {
    Serial.println("No file open.");
    return 0;
  }
  // did we find the movi list?
  if (mMoviListPosition == 0) {
    Serial.println("No movi list found.");
    return 0;
  }
  // get the next chunk of data from the list
  ChunkHeader header;
  while (mMoviListLength > 0)
  {
    readChunk(mFile, &header);
    mMoviListLength -= 8;
    bool isVideoChunk = strncmp(header.chunkId, "00dc", 4) == 0;
    bool isAudioChunk = strncmp(header.chunkId, "01wb", 4) == 0;
    if (mRequiredChunkType == AVIChunkType::VIDEO && isVideoChunk ||
        mRequiredChunkType == AVIChunkType::AUDIO && isAudioChunk)
    {
      // we've got the required chunk - copy it into the provided buffer
      // reallocate the buffer if necessary
      if (header.chunkSize > bufferLength)
      {
        *buffer = (uint8_t *)realloc(*buffer, header.chunkSize);
      }
      // copy the chunk data
      fread(*buffer, header.chunkSize, 1, mFile);
      mMoviListLength -= header.chunkSize;
      // handle any padding bytes
      if (header.chunkSize % 2 != 0)
      {
        fseek(mFile, 1, SEEK_CUR);
        mMoviListLength--;
      }
      return header.chunkSize;
    }
    else
    {
      // the data is not what was required - skip over the chunk
      fseek(mFile, header.chunkSize, SEEK_CUR);
      mMoviListLength -= header.chunkSize;
    }
    // handle any padding bytes
    if (header.chunkSize % 2 != 0)
    {
      fseek(mFile, 1, SEEK_CUR);
      mMoviListLength--;
    }
  }
  // no more chunks
  Serial.println("No more data");
  return 0;
}