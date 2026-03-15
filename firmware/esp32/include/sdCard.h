#ifndef SDCARD_H
#define SDCARD_H

#include <SD.h>
#include <SPI.h>
#include <FS.h>


#define SAMPLE_RATE 48000  // DVD quality
#define WAVE_HEADER_SIZE 44
#define MAX_FILES 100

class SDCard {
public:
    SDCard(int csPin, int sckPin, int misoPin, int mosiPin);
    bool begin();
    void write(const uint8_t* buf, size_t size);
    void closeCurrentFile();
    void writeWavHeader(int sampleRate, int bitsPerSample, int channels, int dataSize);
    boolean initNewFile();
    String readFile(const char* fileName);
    bool writeFile(const char* fileName, const String& content, bool append = false);
    bool deleteFile(const char* fileName);
    int getFilesCount();
    const char* getFileName(int index);
    String getCurrentFileName() const { return currentFileName; }

private:
    int csPin;
    int sckPin;
    int misoPin;
    int mosiPin;
    File currentFile;
    int fileIndex = 1;
    String currentFileName;
    char* fileNames[MAX_FILES];
    int fileCount = 0;

};

#endif
