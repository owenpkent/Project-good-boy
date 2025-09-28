#include <Preferences.h>
#include "sys/config.h"
#include "config.h"
#include "sys/logging.h"

bool AppConfig::begin() {
  // Derive a default device name from project and MAC
  char mac[13];
  snprintf(mac, sizeof(mac), "%012llX", ESP.getEfuseMac());
  device_name = std::string(PROJECT_NAME) + "-" + std::string(mac);
  return true;
}

bool AppConfig::load() {
  Preferences prefs;
  if (!prefs.begin(NAMESPACE, true)) {
    LOGE("CONFIG", "NVS begin (ro) failed");
    return false;
  }
  String ssid = prefs.getString("wifi_ssid", WIFI_SSID);
  String pass = prefs.getString("wifi_pass", WIFI_PASS);
  wifi_ssid = std::string(ssid.c_str());
  wifi_pass = std::string(pass.c_str());
  String dn = prefs.getString("device_name", device_name.c_str());
  if (dn.length()) device_name = std::string(dn.c_str());
  prefs.end();
  LOGI("CONFIG", "Loaded. device_name=%s ssid=%s", device_name.c_str(), wifi_ssid.c_str());
  return true;
}

bool AppConfig::save() const {
  Preferences prefs;
  if (!prefs.begin(NAMESPACE, false)) {
    LOGE("CONFIG", "NVS begin (rw) failed");
    return false;
  }
  prefs.putString("wifi_ssid", wifi_ssid.c_str());
  prefs.putString("wifi_pass", wifi_pass.c_str());
  prefs.putString("device_name", device_name.c_str());
  prefs.end();
  LOGI("CONFIG", "Saved.");
  return true;
}
