#include "main.h"
#include "recorderServer.h"
#include "recorderPreferences.h"
#include "rtc.h"

#define I2S_BCK_PIN D1
#define I2S_WS_PIN D2
#define I2S_SD_PIN D0

#define SD_CS_PIN D7
#define SD_MOSI_PIN MOSI
#define SD_MISO_PIN MISO
#define SD_SCK_PIN SCK

#define BUTTON_PIN D3
#define LED_PIN D5

#define I2S_NUM I2S_NUM_0

bool isRecording = false;
unsigned long lastBlinkTime = 0;

time_t lastFileSwitch = 0;
time_t lastActivation = 0;
bool sdPresent = false;

Led led(LED_PIN);
BlinkMode slowMode(40, 2000, 0);      // 100ms on, 2s off, infinite
BlinkMode fastMode(100, 100, 0);       // 100ms on/off, infinite
BlinkMode twiceMode(400, 400, 2);      // 400ms on/off, 2 blinks max
BlinkMode threeTimesMode(100, 100, 3);    // 200ms on/off, 3 blinks max
SDCard sdCard(SD_CS_PIN, SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
Button button(BUTTON_PIN);
RecorderPreferences* preferences = nullptr;
RecorderServer* recorderServer = nullptr;
RTC* rtc = nullptr;
 
const i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_STAND_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 8,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

const i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_BCK_PIN,
  .ws_io_num = I2S_WS_PIN,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_SD_PIN
};

// Forward declarations
void startRecording();
void stopRecording();

void setup() {
  Serial.begin(115200);
  delay(3000); // Allow time for Serial to initialize
  // Make sure wifi is off to save power and avoid SD card interference
  WiFi.mode(WIFI_OFF);
  // Turn off Bluetooth as well to save power and avoid interference
  btStop();
  
  // Initialize preferences
  preferences = new RecorderPreferences();
  
  // Pass preferences to LED
  led.setPreferences(preferences);

  // Initialize RTC
  rtc = new RTC();
  log_i("RTC initialized (waiting for time synchronization)");

  // Initialize SDCard with preferences and RTC
  if (!sdCard.begin(preferences, rtc)) {
    log_e("SD Card initialization failed");
    sdPresent = false;
    led.setMode(fastMode);
  } else {
    sdPresent = true;
    Serial.println("SD Card Ready.");
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
    log_i("System ready. Click button to start recording.");
    led.setMode(twiceMode);
    lastActivation = millis();
  }
  
  // Initialize RecorderServer with RTC
  recorderServer = new RecorderServer(preferences, &sdCard, rtc);
  recorderServer->setRecordingCallbacks(
    []() { startRecording(); },
    []() { stopRecording(); },
    []() -> bool { return isRecording; }
  );
  
  // Check if server should auto-start after restart
  if (preferences->getSettingBool(PREF_RESTART_SERVER)) {
    log_i("Auto-starting server after restart");
    recorderServer->start();
    led.setMode(threeTimesMode);
    preferences->setSetting(PREF_RESTART_SERVER, false);  // Reset the flag
  }
}

void startRecording() { 
  log_i("Starting recording.");
  lastFileSwitch = millis();
  if (!sdCard.initNewFile()) {
    led.setMode(fastMode);
    log_e("Starting recording failed.");
    return;
  }
  isRecording = true;
  led.setMode(slowMode);
  log_i("Recording started.");
}

void stopRecording() {
  log_i("Stopping recording.");
  sdCard.closeCurrentFile();
  isRecording = false;
  led.off();
  log_i("Recording stopped.");
}

void loop() {  
  button.run();
  led.run();
  recorderServer->run();
  
 
  if (!sdPresent) {
    return;
  }  

  // when not recording for a while, enter light sleep mode to save power.
  // Keep delay long enough, sleep messes up the Serial connection 
  // and prevents uploading new code even after reset, until the next unplug/plug.
  int sleepDelay = preferences->getSettingInt(PREF_SLEEP_DELAY_S) * 1000;
  if (millis() - lastActivation > sleepDelay && !recorderServer->isRunning()) {
    log_i("No activity, going to light sleep...");
    fflush(stdout);
    gpio_wakeup_enable(GPIO_NUM_5, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();
    // Beware, after this, USB Serial no longer works (logs, upload, ...) 
    // until the next time it is unplugged / plugged in.
    lastActivation = millis();

    // This does not work :( 
    // vTaskDelay(pdMS_TO_TICKS(10));
    // periph_module_reset(PERIPH_USB_DEVICE_MODULE);
    // vTaskDelay(pdMS_TO_TICKS(10));
    // Serial.begin(115200);
    // delay(3000);
    // printf("Woke up from light sleep!\n");
    // fflush(stdout);
  }
  
  ButtonState buttonState = button.readState();
  if (buttonState == ButtonState::SHORT_PRESS) {
    if (isRecording) {
      stopRecording();
    } else {
      startRecording();
    }
  } else if (buttonState == ButtonState::LONG_PRESS) {
    if (recorderServer->isRunning()) {
      led.setMode(threeTimesMode);
      recorderServer->stop();
      log_i("Web server stopped.");
      if (isRecording) {
        led.setMode(slowMode);
      }
    } else {
      led.setMode(threeTimesMode);
      recorderServer->start();
      log_i("Web server started.");
    }
    lastActivation = millis();
  } else if (buttonState == ButtonState::VERY_LONG_PRESS) {
    log_i("Button held for 6+ seconds. Resetting ESP32...");
    delay(500);
    ESP.restart();
  }
  

  if (isRecording) { 
    size_t bytesRead;
    int32_t i2sBuffer[256]; 
    lastActivation = millis();
    
    i2s_read(I2S_NUM, (void *)i2sBuffer, sizeof(i2sBuffer), &bytesRead, portMAX_DELAY);
    
    int16_t samples16[256];
    int samplesCount = bytesRead / 4;
    
    for (int i = 0; i < samplesCount; i++) {
      samples16[i] = (int16_t)(i2sBuffer[i] >> 14);
    }
    
    sdCard.write((uint8_t *)samples16, samplesCount * 2);

    // New file every X seconds.
    int fileSwitchDelay = preferences->getSettingInt(PREF_FILE_SWITCH_DELAY_S) * 1000;
    if (millis() - lastFileSwitch > (unsigned long)fileSwitchDelay) { 
      Serial.print("Switching recording file.");
      stopRecording();
      startRecording();
    }
  }
}
