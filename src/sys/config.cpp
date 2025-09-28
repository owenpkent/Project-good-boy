#include <Preferences.h>
#include "sys/config.h"
#include "config.h"
#include "sys/logging.h"

bool AppConfig::begin() {
  // Derive a default device name from project and MAC
  char mac[13];
  snprintf(mac, sizeof(mac), "%012llX", ESP.getEfuseMac());
  device_name = String(PROJECT_NAME) + "-" + String(mac);
  return true;
}

bool AppConfig::load() {
  Preferences prefs;
  if (!prefs.begin(NAMESPACE, true)) {
    LOGE("CONFIG", "NVS begin (ro) failed");
    return false;
  }
  wifi_ssid = prefs.getString("wifi_ssid", WIFI_SSID);
  wifi_pass = prefs.getString("wifi_pass", WIFI_PASS);
  String dn = prefs.getString("device_name", device_name);
  if (dn.length()) device_name = dn;
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
  prefs.putString("wifi_ssid", wifi_ssid);
  prefs.putString("wifi_pass", wifi_pass);
  prefs.putString("device_name", device_name);
  prefs.end();
  LOGI("CONFIG", "Saved.");
  return true;
}
