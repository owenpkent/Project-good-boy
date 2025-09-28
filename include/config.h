#pragma once

// Project identity
#define PROJECT_NAME "Project-good-boy"
#define FW_VERSION   "0.1.0"

// Wi-Fi credentials (leave empty to rely on NVS/provisioning)
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

// NTP server
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

// Features
#ifndef OTA_ENABLE
#define OTA_ENABLE 1
#endif

// Logging level: 0=ERROR,1=WARN,2=INFO,3=DEBUG
#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif
