#pragma once

#include "cyd_config.h"
#include <Arduino.h>
#include <FS.h>
#include <SPI.h>

using fs::File;

#if defined(CYD_SD_USE_SD_MMC)
  #include <SD_MMC.h>

class S3SdMmcCompat {
public:
    bool begin(int csPin = -1,
               SPIClass& spi = SPI,
               uint32_t frequency = 4000000,
               const char* mountpoint = "/sd",
               uint8_t maxOpenFiles = 5,
               bool formatIfMountFailed = false);
    void end();
    File open(const char* path, const char* mode = FILE_READ, const bool create = false);
    File open(const String& path, const char* mode = FILE_READ, const bool create = false);
    bool exists(const char* path);
    bool exists(const String& path);
    bool mkdir(const char* path);
    bool mkdir(const String& path);
    bool remove(const char* path);
    bool remove(const String& path);
    bool rmdir(const char* path);
    bool rmdir(const String& path);
    bool rename(const char* pathFrom, const char* pathTo);
    bool rename(const String& pathFrom, const String& pathTo);
    uint64_t cardSize();
    sdcard_type_t cardType();
    uint64_t totalBytes();
    uint64_t usedBytes();
    bool ready() const { return initialized; }

private:
    bool initialized = false;
};

extern S3SdMmcCompat SD;

#else
  #include <SD.h>
#endif

bool storageBegin();
bool storageReady();
const char* storageBackendName();
