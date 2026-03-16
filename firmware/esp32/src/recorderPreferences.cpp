#include "recorderPreferences.h"

// Initialize static metadata array
const RecorderPreferences::SettingMetadata RecorderPreferences::settingsMetadata[] = {
  {PREF_DEVICE_NAME, "string", PREF_DEVICE_NAME_DEFAULT, 0, 0.0f, false, "Device Name"},
  {PREF_DISABLE_LED, "bool", nullptr, 0, 0.0f, PREF_DISABLE_LED_DEFAULT, "Disable LED"},
  {PREF_RESTART_SERVER, "bool", nullptr, 0, 0.0f, PREF_RESTART_SERVER_DEFAULT, "Restart Server on Reset"},
  {PREF_SLEEP_DELAY_S, "int", nullptr, PREF_SLEEP_DELAY_DEFAULT, 0.0f, false, "Sleep Delay (s)"},
  {PREF_FILE_SWITCH_DELAY_S, "int", nullptr, PREF_FILE_SWITCH_DELAY_DEFAULT, 0.0f, false, "File Switch Delay (s)"},
  {PREF_AP_SSID, "string", PREF_AP_SSID_DEFAULT, 0, 0.0f, false, "Access Point SSID"},
  {PREF_AP_PASSWORD, "string", PREF_AP_PASSWORD_DEFAULT, 0, 0.0f, false, "Access Point Password"},
  {PREF_FILE_INDEX, "int", nullptr, PREF_FILE_INDEX_DEFAULT, 0.0f, false, "File Index"},
};

const size_t RecorderPreferences::settingsMetadataSize = 
    sizeof(RecorderPreferences::settingsMetadata) / sizeof(RecorderPreferences::SettingMetadata);


RecorderPreferences::RecorderPreferences() {
    // Initialize Preferences with the namespace
    settings.begin(PREFERENCES_NAMESPACE, false);
}


RecorderPreferences::~RecorderPreferences() {
    // Close preferences
    settings.end();
}


void RecorderPreferences::setSetting(const char* name, const String& value) {
    const SettingMetadata* metadata = findMetadata(name);
    if (metadata && String(metadata->type) == "string") {
        settings.putString(name, value);
    }
}


void RecorderPreferences::setSetting(const char* name, int value) {
    const SettingMetadata* metadata = findMetadata(name);
    if (metadata && String(metadata->type) == "int") {
        settings.putInt(name, value);
    }
}


void RecorderPreferences::setSetting(const char* name, float value) {
    const SettingMetadata* metadata = findMetadata(name);
    if (metadata && String(metadata->type) == "float") {
        settings.putFloat(name, value);
    }
}


void RecorderPreferences::setSetting(const char* name, bool value) {
    const SettingMetadata* metadata = findMetadata(name);
    if (metadata && String(metadata->type) == "bool") {
        settings.putBool(name, value);
    }
}


String RecorderPreferences::getSettingString(const char* name) {
    const SettingMetadata* metadata = findMetadata(name);
    if (!metadata) {
        return "";
    }
    
    if (String(metadata->type) == "string") {
        return settings.getString(name, metadata->defaultStr);
    }
    
    return "";
}


int RecorderPreferences::getSettingInt(const char* name) {
    const SettingMetadata* metadata = findMetadata(name);
    if (!metadata) {
        return 0;
    }
    
    if (String(metadata->type) == "int") {
        return settings.getInt(name, metadata->defaultInt);
    }
    
    return 0;
}


float RecorderPreferences::getSettingFloat(const char* name) {
    const SettingMetadata* metadata = findMetadata(name);
    if (!metadata) {
        return 0.0f;
    }
    
    if (String(metadata->type) == "float") {
        return settings.getFloat(name, metadata->defaultFloat);
    }
    
    return 0.0f;
}


bool RecorderPreferences::getSettingBool(const char* name) {
    const SettingMetadata* metadata = findMetadata(name);
    if (!metadata) {
        return false;
    }
    
    if (String(metadata->type) == "bool") {
        return settings.getBool(name, metadata->defaultBool);
    }
    
    return false;
}


const RecorderPreferences::SettingMetadata* RecorderPreferences::getSettingsMetadata(size_t& count) {
    count = settingsMetadataSize;
    return settingsMetadata;
}


const RecorderPreferences::SettingMetadata* RecorderPreferences::findMetadata(const char* name) {
    for (size_t i = 0; i < settingsMetadataSize; i++) {
        if (strcmp(settingsMetadata[i].name, name) == 0) {
            return &settingsMetadata[i];
        }
    }
    return nullptr;
}
