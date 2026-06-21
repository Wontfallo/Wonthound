#include "storage.h"

#if defined(CYD_SD_USE_SD_MMC)

S3SdMmcCompat SD;

bool S3SdMmcCompat::begin(int csPin,
                          SPIClass& spi,
                          uint32_t frequency,
                          const char* mountpoint,
                          uint8_t maxOpenFiles,
                          bool formatIfMountFailed) {
    (void)csPin;
    (void)spi;
    (void)frequency;

    if (initialized) return true;

    if (!SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0, SD_MMC_D1, SD_MMC_D2, SD_MMC_D3)) {
        Serial.println("[SD] SD_MMC.setPins failed");
        return false;
    }

    initialized = SD_MMC.begin(mountpoint, false, formatIfMountFailed, SDMMC_FREQ_DEFAULT, maxOpenFiles);
    if (!initialized) {
        Serial.println("[SD] SD_MMC 4-bit mount failed; retrying 1-bit");
        initialized = SD_MMC.begin(mountpoint, true, formatIfMountFailed, SDMMC_FREQ_DEFAULT, maxOpenFiles);
    }

    Serial.printf("[SD] %s %s\n", storageBackendName(), initialized ? "ready" : "not mounted");
    return initialized;
}

void S3SdMmcCompat::end() {
    if (initialized) {
        SD_MMC.end();
        initialized = false;
    }
}

File S3SdMmcCompat::open(const char* path, const char* mode, const bool create) {
    if (!initialized && !begin()) return File();
    return SD_MMC.open(path, mode, create);
}

File S3SdMmcCompat::open(const String& path, const char* mode, const bool create) {
    return open(path.c_str(), mode, create);
}

bool S3SdMmcCompat::exists(const char* path) {
    if (!initialized && !begin()) return false;
    return SD_MMC.exists(path);
}

bool S3SdMmcCompat::exists(const String& path) {
    return exists(path.c_str());
}

bool S3SdMmcCompat::mkdir(const char* path) {
    if (!initialized && !begin()) return false;
    return SD_MMC.mkdir(path);
}

bool S3SdMmcCompat::mkdir(const String& path) {
    return mkdir(path.c_str());
}

bool S3SdMmcCompat::remove(const char* path) {
    if (!initialized && !begin()) return false;
    return SD_MMC.remove(path);
}

bool S3SdMmcCompat::remove(const String& path) {
    return remove(path.c_str());
}

bool S3SdMmcCompat::rmdir(const char* path) {
    if (!initialized && !begin()) return false;
    return SD_MMC.rmdir(path);
}

bool S3SdMmcCompat::rmdir(const String& path) {
    return rmdir(path.c_str());
}

bool S3SdMmcCompat::rename(const char* pathFrom, const char* pathTo) {
    if (!initialized && !begin()) return false;
    return SD_MMC.rename(pathFrom, pathTo);
}

bool S3SdMmcCompat::rename(const String& pathFrom, const String& pathTo) {
    return rename(pathFrom.c_str(), pathTo.c_str());
}

uint64_t S3SdMmcCompat::cardSize() {
    return initialized ? SD_MMC.cardSize() : 0;
}

sdcard_type_t S3SdMmcCompat::cardType() {
    return initialized ? SD_MMC.cardType() : CARD_NONE;
}

uint64_t S3SdMmcCompat::totalBytes() {
    return initialized ? SD_MMC.totalBytes() : 0;
}

uint64_t S3SdMmcCompat::usedBytes() {
    return initialized ? SD_MMC.usedBytes() : 0;
}

bool storageBegin() {
    return SD.begin();
}

bool storageReady() {
    return SD.ready();
}

const char* storageBackendName() {
    return "SD_MMC";
}

#else

bool storageBegin() {
    return SD.begin(SD_CS);
}

bool storageReady() {
    return true;
}

const char* storageBackendName() {
    return "SPI SD";
}

#endif
