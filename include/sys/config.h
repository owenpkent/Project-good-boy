#pragma once
#include <Arduino.h>

class AppConfig {
public:
  String device_name;
  String wifi_ssid;
  String wifi_pass;

  bool begin();
  bool load();
  bool save() const;

private:
  static constexpr const char* NAMESPACE = "app";
};
