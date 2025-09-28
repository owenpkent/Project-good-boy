#pragma once
#include "sys/compat.h"

class AppConfig {
public:
  Str device_name;
  Str wifi_ssid;
  Str wifi_pass;

  bool begin();
  bool load();
  bool save() const;

private:
  static constexpr const char* NAMESPACE = "app";
};
