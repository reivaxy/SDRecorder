#ifndef RECORDER_PREFERENCES_H
#define RECORDER_PREFERENCES_H

#include <Preferences.h>
#include <Arduino.h>

// Preference namespace
#define PREFERENCES_NAMESPACE "SDRecorder"

// String preferences
#define PREF_AP_SSID "ap_ssid"
#define PREF_AP_SSID_DEFAULT "SDRecorder"

#define PREF_AP_PASSWORD "ap_password"
#define PREF_AP_PASSWORD_DEFAULT "12345678"

#define PREF_DEVICE_NAME "device_name"
#define PREF_DEVICE_NAME_DEFAULT "SDRecorder"

// Integer preferences
#define PREF_SLEEP_DELAY_S "sleep_delay"
#define PREF_SLEEP_DELAY_DEFAULT 30

#define PREF_FILE_SWITCH_DELAY_S "switch_delay"
#define PREF_FILE_SWITCH_DELAY_DEFAULT 60

// Boolean preferences
#define PREF_DISABLE_LED "disable_led"
#define PREF_DISABLE_LED_DEFAULT false

#define PREF_RESTART_SERVER "restart_server"
#define PREF_RESTART_SERVER_DEFAULT false

// File index preference
#define PREF_FILE_INDEX "file_index"
#define PREF_FILE_INDEX_DEFAULT 1



class RecorderPreferences {
public:
    // Metadata structure for each setting
    struct SettingMetadata {
        const char* name;           // Setting name/key
        const char* type;           // "string", "int", "float", or "bool"
        const char* defaultStr;     // Default value for string type
        int defaultInt;             // Default value for int type
        float defaultFloat;         // Default value for float type
        bool defaultBool;           // Default value for bool type
        const char* label;          // Display label
    };
    
    RecorderPreferences();
    ~RecorderPreferences();
    
    // Setter for any preference type
    void setSetting(const char* name, const String& value);
    void setSetting(const char* name, int value);
    void setSetting(const char* name, float value);
    void setSetting(const char* name, bool value);
    
    // Getter for any preference type
    String getSettingString(const char* name);
    int getSettingInt(const char* name);
    float getSettingFloat(const char* name);
    bool getSettingBool(const char* name);
    
    // Get metadata list
    static const SettingMetadata* getSettingsMetadata(size_t& count);
    
private:
    Preferences settings;
    
    static const SettingMetadata settingsMetadata[];
    static const size_t settingsMetadataSize;
    
    // Helper method to find metadata by name
    const SettingMetadata* findMetadata(const char* name);
};

#endif // RECORDER_PREFERENCES_H