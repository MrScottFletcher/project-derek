#pragma once
// Cluster Logic Board Rev G - Hosyond ESP32-S3 N16R8
// GPIO numbers are Arduino GPIO numbers, not physical carrier header positions.

constexpr int PIN_ID0 = 4;
constexpr int PIN_ID1 = 5;
constexpr int PIN_ID2 = 6;
constexpr int PIN_ID3 = 7;

constexpr int PIN_I2C_SDA = 8;
constexpr int PIN_I2C_SCL = 9;
constexpr int PIN_LED_DATA = 10;

constexpr int PIN_PIR_LEFT   = 11;
constexpr int PIN_PIR_CENTER = 12;
constexpr int PIN_PIR_RIGHT  = 13;
constexpr int PIN_PCA_OE_N   = 14;

constexpr int PIN_DF2_TX = 15;  // ESP TX -> R6 -> DFPlayer #2 RX
constexpr int PIN_DF2_RX = 16;  // DFPlayer #2 TX -> ESP RX
constexpr int PIN_DF1_TX = 17;  // ESP TX -> R5 -> DFPlayer #1 RX
constexpr int PIN_DF1_RX = 18;  // DFPlayer #1 TX -> ESP RX
