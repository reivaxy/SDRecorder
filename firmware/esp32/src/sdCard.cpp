#include "sdCard.h"

SDCard::SDCard(int csPin, int sckPin, int misoPin, int mosiPin) : csPin(csPin), sckPin(sckPin), misoPin(misoPin), mosiPin(mosiPin) {}

bool SDCard::begin() {
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
  while (SD.exists("/rec" + String(fileIndex) + ".wav")) {
    fileIndex++;
  }
  currentFileName = "/rec" + String(fileIndex) + ".wav";
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
