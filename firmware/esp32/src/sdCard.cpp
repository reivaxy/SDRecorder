#include "sdCard.h"

SDCard::SDCard(int csPin, int sckPin, int misoPin, int mosiPin)
    : csPin(csPin), sckPin(sckPin), misoPin(misoPin), mosiPin(mosiPin), fileCount(0) {
    memset(fileNames, 0, sizeof(fileNames));
}

bool SDCard::begin(RecorderPreferences* prefs, RTC* rtc) {
    preferences = prefs;
    this->rtc = rtc;
    loadFileIndex();
    SPI.begin(sckPin, misoPin, mosiPin, csPin);
    return SD.begin(csPin);
}

void SDCard::write(const uint8_t* buf, size_t size) {
    currentFile.write(buf, size);
}

void SDCard::closeCurrentFile() {
  unsigned long fileSize = currentFile.size() - WAVE_HEADER_SIZE;
  currentFile.seek(0);

  writeWavHeader(SAMPLE_RATE, 16, 1, fileSize);
  currentFile.close();
  log_i("File closed: %s", currentFileName.c_str());
}

boolean SDCard::initNewFile() {
  char fileName[32];
  while (true) {
    sprintf(fileName, "/rec%03d.wav", fileIndex);
    if (!SD.exists(fileName)) {
      break;
    }
    fileIndex++;
  }
  saveFileIndex();
  currentFileName = String(fileName);
  log_i("Initializing new file: %s", currentFileName.c_str());
  currentFile = SD.open(currentFileName.c_str(), FILE_WRITE);
  if (!currentFile) {
    log_e("Could not open file %s for writing", currentFileName.c_str());
  } else {
    log_i("File %s opened successfully", currentFileName.c_str());
    writeWavHeader(SAMPLE_RATE, 16, 1, 0);
  }
  return !!currentFile;
}

void SDCard::writeWavHeader(int sampleRate, int bitsPerSample, int channels, int dataSize) {
    byte header[WAVE_HEADER_SIZE];
    int fileSize = dataSize + WAVE_HEADER_SIZE - 8;
    int byteRate = sampleRate * channels * (bitsPerSample / 8);

    header[0] = 'R';
    header[1] = 'I';
    header[2] = 'F';
    header[3] = 'F';
    header[4] = (byte)(fileSize & 0xFF);
    header[5] = (byte)((fileSize >> 8) & 0xFF);
    header[6] = (byte)((fileSize >> 16) & 0xFF);
    header[7] = (byte)((fileSize >> 24) & 0xFF);
    header[8] = 'W';
    header[9] = 'A';
    header[10] = 'V';
    header[11] = 'E';
    header[12] = 'f';
    header[13] = 'm';
    header[14] = 't';
    header[15] = ' ';
    header[16] = 16;
    header[17] = 0;
    header[18] = 0;
    header[19] = 0;
    header[20] = 1;
    header[21] = 0;
    header[22] = channels;
    header[23] = 0;
    header[24] = (byte)(sampleRate & 0xFF);
    header[25] = (byte)((sampleRate >> 8) & 0xFF);
    header[26] = (byte)((sampleRate >> 16) & 0xFF);
    header[27] = (byte)((sampleRate >> 24) & 0xFF);
    header[28] = (byte)(byteRate & 0xFF);
    header[29] = (byte)((byteRate >> 8) & 0xFF);
    header[30] = (byte)((byteRate >> 16) & 0xFF);
    header[31] = (byte)((byteRate >> 24) & 0xFF);
    header[32] = (byte)(channels * (bitsPerSample / 8));
    header[33] = 0;
    header[34] = bitsPerSample;
    header[35] = 0;
    header[36] = 'd';
    header[37] = 'a';
    header[38] = 't';
    header[39] = 'a';
    header[40] = (byte)(dataSize & 0xFF);
    header[41] = (byte)((dataSize >> 8) & 0xFF);
    header[42] = (byte)((dataSize >> 16) & 0xFF);
    header[43] = (byte)((dataSize >> 24) & 0xFF);

    currentFile.write(header, WAVE_HEADER_SIZE);
}

String SDCard::readFile(const char* fileName) {
    log_i("Reading file: %s", fileName);
    
    if (!SD.exists(fileName)) {
        log_w("File does not exist: %s", fileName);
        return String();
    }
    
    File file = SD.open(fileName, FILE_READ);
    if (!file) {
        log_e("Could not open file for reading: %s", fileName);
        return String();
    }
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    
    log_i("File read successfully: %s", fileName);
    return content;
}

bool SDCard::writeFile(const char* fileName, const String& content, bool append) {
    log_i("Writing to file: %s (append: %d)", fileName, append ? 1 : 0);
    
    File file = SD.open(fileName, append ? FILE_APPEND : FILE_WRITE);
    if (!file) {
        log_e("Could not open file for writing: %s", fileName);
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    if (bytesWritten > 0) {
        log_i("File written successfully: %s (%d bytes)", fileName, bytesWritten);
        return true;
    } else {
        log_w("No bytes written to file: %s", fileName);
        return false;
    }
}

bool SDCard::deleteFile(const char* fileName) {
    log_i("Deleting file: %s", fileName);
    
    if (!SD.exists(fileName)) {
        log_w("File does not exist: %s", fileName);
        return false;
    }
    
    if (SD.remove(fileName)) {
        log_i("File deleted successfully: %s", fileName);
        return true;
    } else {
        log_e("Failed to delete file: %s", fileName);
        return false;
    }
}

int SDCard::getFilesCount() {
    // Free previously scanned file names
    for (int i = 0; i < fileCount; i++) {
        if (fileNames[i]) {
            free(fileNames[i]);
            fileNames[i] = nullptr;
        }
    }
    fileCount = 0;

    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        log_e("Failed to open SD root directory");
        return 0;
    }

    File entry = root.openNextFile();
    while (entry && fileCount < MAX_FILES) {
        if (!entry.isDirectory()) {
            String name = String(entry.name());
            // Normalize: ensure leading '/'
            if (!name.startsWith("/")) {
                name = "/" + name;
            }
            // Only include WAV files (case-insensitive)
            String nameLower = name;
            nameLower.toLowerCase();
            if (nameLower.endsWith(".wav")) {
                fileNames[fileCount] = strdup(name.c_str());
                fileCount++;
            }
        }
        entry = root.openNextFile();
    }
    root.close();

    log_i("Found %d WAV files on SD card", fileCount);
    return fileCount;
}

const char* SDCard::getFileName(int index) {
    if (index < 0 || index >= fileCount) {
        return nullptr;
    }
    return fileNames[index];
}

void SDCard::saveFileIndex() {
    if (preferences) {
        preferences->setSetting(PREF_FILE_INDEX, fileIndex);
        log_i("File index saved to preferences: %d", fileIndex);
    }
}

void SDCard::loadFileIndex() {
    if (preferences) {
        fileIndex = preferences->getSettingInt(PREF_FILE_INDEX);
        log_i("File index loaded from preferences: %d", fileIndex);
    } else {
        fileIndex = PREF_FILE_INDEX_DEFAULT;
        log_w("Preferences not available, using default file index: %d", fileIndex);
    }
}