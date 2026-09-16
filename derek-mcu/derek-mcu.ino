#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <esp_now.h>

#ifndef ARDUINO_ARCH_ESP32
#error "This sketch targets ESP32-class boards."
#endif

namespace {

constexpr bool kEnableSerialLogs = true;
constexpr uint8_t kSoftwareVersionMajor = 1;
constexpr uint8_t kSoftwareVersionMinor = 0;
constexpr uint8_t kSoftwareVersionRevision = 0;
constexpr uint8_t kProtocolVersion = 1;
constexpr uint16_t kProtocolMagic = 0xD311;
constexpr uint8_t kMaxClusters = 15;
constexpr uint8_t kMaxDerricks = 8;
constexpr uint32_t kDiscoveryIntervalMs = 2000;
constexpr uint32_t kCommandIntervalMs = 100;
constexpr uint32_t kStatusPageIntervalMs = 1000;
constexpr uint32_t kClusterOfflineMs = 3000;
constexpr uint32_t kBootDerekTestWaitMs = 5000;
constexpr uint32_t kBootDerekTestStepMs = 1200;
constexpr uint16_t kOscListenPort = 7700;
constexpr uint16_t kOscStatusPort = 9000;
constexpr size_t kOscPacketBufferSize = 768;
constexpr bool kEnableOscDebugLogs = true;
constexpr uint32_t kOscDebugLogIntervalMs = 1000;
constexpr bool kEnableRadioDebugLogs = true;
constexpr uint32_t kRadioDebugLogIntervalMs = 1000;
constexpr uint8_t kClusterControllerChannels = 14;
constexpr uint8_t kChannelsPerDerek = 11;
constexpr uint8_t kDerekChannelsStart = kClusterControllerChannels + 1;
constexpr uint16_t kDerekOutputChannels = kMaxDerricks * kChannelsPerDerek;
constexpr uint16_t kChannelsPerCluster = kClusterControllerChannels + kDerekOutputChannels;
constexpr uint16_t kControllerActiveDerricksChannel = 1;
constexpr uint16_t kControllerApplyOutputsChannel = 2;
constexpr uint16_t kControllerEmergencyHideChannel = 3;
constexpr uint16_t kControllerRequestStatusChannel = 4;
constexpr uint16_t kControllerDiscoveryChannel = 5;
constexpr uint16_t kControllerAudioATrackMsbChannel = 6;
constexpr uint16_t kControllerAudioATrackLsbChannel = 7;
constexpr uint16_t kControllerAudioATriggerChannel = 8;
constexpr uint16_t kControllerAudioBTrackMsbChannel = 9;
constexpr uint16_t kControllerAudioBTrackLsbChannel = 10;
constexpr uint16_t kControllerAudioBTriggerChannel = 11;
constexpr uint16_t kControllerMovementSpeedChannel = 12;
constexpr uint16_t kControllerPirCenterChannel = 13;
constexpr uint16_t kControllerPirRightChannel = 14;
constexpr int8_t kI2cSdaPin = 8;
constexpr int8_t kI2cSclPin = 9;
constexpr uint8_t kOledAddress = 0x3C;
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Fill these in for the router/network used by the QLC+ computer.
// ESP-NOW peers must share the same Wi-Fi radio channel as this connection.
constexpr char kWifiSsid[] = "DerekWifi";
constexpr char kWifiPassword[] = "derekderek";

// OLED wiring on this board: GND, VCC, SCL -> GPIO9, SDA/SCA -> GPIO8.
constexpr int8_t kButtonNextPin = -1;
constexpr int8_t kButtonPrevPin = -1;

enum MessageKind : uint8_t {
  kMessageRegister = 1,
  kMessageCommand = 2,
  kMessageStatus = 3,
};

enum CommandFlags : uint8_t {
  kCommandFlagApplyOutputs = 1 << 0,
  kCommandFlagAudioA = 1 << 1,
  kCommandFlagAudioB = 1 << 2,
  kCommandFlagRequestStatus = 1 << 3,
  kCommandFlagEmergencyHide = 1 << 4,
  kCommandFlagDiscovery = 1 << 5,
};

enum BootDerekTestPhase : uint8_t {
  kBootDerekTestLift = 0,
  kBootDerekTestLeft = 1,
  kBootDerekTestRight = 2,
  kBootDerekTestCenter = 3,
  kBootDerekTestLower = 4,
  kBootDerekTestPhaseCount = 5,
};

enum MovementSpeed : uint8_t {
  kMovementSpeedFast = 0,
  kMovementSpeedMedium = 1,
  kMovementSpeedSlow = 2,
};

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct DerekCommand {
  uint8_t pan;
  uint8_t lift;
  RgbColor eyeColor;
  RgbColor canColor;
  RgbColor exteriorColor;
};

#pragma pack(push, 1)
struct ClusterCommandPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t kind;
  uint8_t clusterId;
  uint16_t sequence;
  uint8_t flags;
  uint8_t activeDerricks;
  uint8_t movementSpeed;
  uint16_t audioTrackA;
  uint16_t audioTrackB;
  DerekCommand derricks[kMaxDerricks];
};

struct ClusterStatusPacket {
  uint16_t magic;
  uint8_t version;
  uint8_t kind;
  uint8_t clusterId;
  uint16_t sequence;
  uint8_t activeDerricks;
  uint8_t pirStateBits;
  uint16_t healthFlags;
  uint32_t uptimeMs;
  uint32_t lastCommandAgeMs;
};
#pragma pack(pop)

struct ClusterRuntime {
  bool occupied;
  bool online;
  uint8_t clusterId;
  uint8_t mac[6];
  uint16_t lastSequence;
  uint8_t lastActiveDerricks;
  uint8_t lastPirBits;
  uint16_t lastHealthFlags;
  uint32_t lastSeenAtMs;
  uint32_t lastStatusUptimeMs;
  uint16_t audioTrackASetting;
  uint16_t audioTrackBSetting;
  bool audioATriggerHigh;
  bool audioBTriggerHigh;
  MovementSpeed movementSpeed;
  ClusterCommandPacket desiredCommand;
};

ClusterRuntime gClusters[kMaxClusters];
uint32_t gLastDiscoveryAtMs = 0;
uint32_t gLastCommandAtMs = 0;
uint32_t gLastStatusPageAtMs = 0;
uint32_t gBootStartedAtMs = 0;
uint32_t gLastBootDerekTestStepAtMs = 0;
uint16_t gSequenceCounter = 1;
uint8_t gCurrentPage = 0;
uint8_t gBootDerekTestClusterCursor = 0;
uint8_t gBootDerekTestDerekCursor = 0;
BootDerekTestPhase gBootDerekTestPhase = kBootDerekTestLift;
bool gBootDerekTestRunning = false;
bool gBootDerekTestDone = false;
bool gOledReady = false;
WiFiUDP gOscUdp;
IPAddress gLastOscRemoteIp;
uint16_t gLastOscRemotePort = 0;
uint32_t gOscPacketsReceived = 0;
uint32_t gOscMessagesAccepted = 0;
uint32_t gOscMessagesRejected = 0;
uint32_t gLastOscPacketAtMs = 0;
uint32_t gLastOscAcceptedAtMs = 0;
uint32_t gLastOscDebugLogAtMs = 0;
uint8_t gLastOscClusterId = 0;
uint16_t gLastOscChannel = 0;
uint8_t gLastOscValue = 0;
char gLastOscAddress[32] = {};
char gLastOscRejectReason[24] = "none";
uint32_t gEspNowPacketsQueued = 0;
uint32_t gEspNowQueueFailures = 0;
uint32_t gEspNowSendSuccesses = 0;
uint32_t gEspNowSendFailures = 0;
uint32_t gLastEspNowQueuedAtMs = 0;
uint32_t gLastEspNowCallbackAtMs = 0;
uint32_t gLastRadioDebugLogAtMs = 0;
uint8_t gLastEspNowClusterId = 0;
uint16_t gLastEspNowSequence = 0;
uint8_t gLastEspNowFlags = 0;
int gLastEspNowQueueResult = 0;
int gLastEspNowSendStatus = 0;

void sendOscStatus(const ClusterRuntime& cluster);

bool addEspNowPeer(const uint8_t* macAddress) {
  if (esp_now_is_peer_exist(macAddress)) {
    return true;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, macAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

void formatMac(const uint8_t* mac, char* buffer, size_t bufferLen) {
  snprintf(
      buffer,
      bufferLen,
      "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0],
      mac[1],
      mac[2],
      mac[3],
      mac[4],
      mac[5]);
}

void formatIpAddress(const IPAddress& ip, char* buffer, size_t bufferLen) {
  snprintf(buffer, bufferLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

bool isProtocolPacketValid(const uint8_t* data, size_t len) {
  if (len < sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t)) {
    return false;
  }

  const uint16_t magic = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  return magic == kProtocolMagic && data[2] == kProtocolVersion;
}

bool writeI2cBytes(uint8_t address, const uint8_t* data, size_t len) {
  Wire.beginTransmission(address);
  Wire.write(data, len);
  return Wire.endTransmission() == 0;
}

void sendOledCommand(uint8_t command) {
  const uint8_t data[2] = {0x00, command};
  writeI2cBytes(kOledAddress, data, sizeof(data));
}

void sendOledData(const uint8_t* data, size_t len) {
  size_t offset = 0;
  while (offset < len) {
    const size_t chunkLen = min<size_t>(len - offset, 15);
    Wire.beginTransmission(kOledAddress);
    Wire.write(0x40);
    Wire.write(data + offset, chunkLen);
    Wire.endTransmission();
    offset += chunkLen;
  }
}

bool initializeOled() {
  const uint8_t initCommands[] = {
      0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
      0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
      0x2E, 0xAF,
  };

  Wire.beginTransmission(kOledAddress);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  for (uint8_t command : initCommands) {
    sendOledCommand(command);
  }
  return true;
}

uint8_t fontColumn(char character, uint8_t column) {
  static const uint8_t kDigits[10][5] = {
      {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
      {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
      {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
      {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
      {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
  };

  static const uint8_t kLetters[26][5] = {
      {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
      {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
      {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
      {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
      {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
      {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
      {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
      {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
      {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
      {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
      {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
      {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
      {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
  };

  if (column >= 5) {
    return 0x00;
  }
  if (character >= 'a' && character <= 'z') {
    character -= 32;
  }
  if (character >= '0' && character <= '9') {
    return kDigits[character - '0'][column];
  }
  if (character >= 'A' && character <= 'Z') {
    return kLetters[character - 'A'][column];
  }

  switch (character) {
    case ':':
      return column == 2 ? 0x36 : 0x00;
    case '.':
      return column == 2 ? 0x40 : 0x00;
    case '/':
      return 0x40 >> column;
    case '-':
      return column > 0 && column < 4 ? 0x08 : 0x00;
    case '_':
      return 0x40;
    case ' ':
    default:
      return 0x00;
  }
}

void writeOledLine(uint8_t page, const char* text) {
  uint8_t pageData[128] = {};
  uint8_t cursor = 0;

  for (uint8_t i = 0; text[i] != '\0' && cursor < sizeof(pageData); ++i) {
    for (uint8_t col = 0; col < 5 && cursor < sizeof(pageData); ++col) {
      pageData[cursor++] = fontColumn(text[i], col);
    }
    if (cursor < sizeof(pageData)) {
      pageData[cursor++] = 0x00;
    }
  }

  sendOledCommand(0xB0 + page);
  sendOledCommand(0x00);
  sendOledCommand(0x10);
  sendOledData(pageData, sizeof(pageData));
}

void clearOled() {
  for (uint8_t page = 0; page < 8; ++page) {
    writeOledLine(page, "");
  }
}

uint8_t countRegisteredClusters() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied) {
      ++count;
    }
  }
  return count;
}

uint8_t countOnlineClusters() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied && gClusters[i].online) {
      ++count;
    }
  }
  return count;
}

const char* bootDerekTestPhaseName() {
  switch (gBootDerekTestPhase) {
    case kBootDerekTestLift:
      return "LIFT";
    case kBootDerekTestLeft:
      return "LEFT";
    case kBootDerekTestRight:
      return "RIGHT";
    case kBootDerekTestCenter:
      return "CENTER";
    case kBootDerekTestLower:
      return "LOWER";
    default:
      return "DONE";
  }
}

void writeOledStatusPage(uint32_t nowMs) {
  if (!gOledReady) {
    return;
  }

  char line[22] = {};
  snprintf(
      line,
      sizeof(line),
      "MCU V%u.%u.%u S%u",
      kSoftwareVersionMajor,
      kSoftwareVersionMinor,
      kSoftwareVersionRevision,
      static_cast<unsigned>(gCurrentPage));
  writeOledLine(0, line);

  snprintf(line, sizeof(line), "NODES %u ONLINE %u", countRegisteredClusters(), countOnlineClusters());
  writeOledLine(1, line);

  if (WiFi.status() == WL_CONNECTED) {
    char ipBuffer[16] = {};
    formatIpAddress(WiFi.localIP(), ipBuffer, sizeof(ipBuffer));
    snprintf(line, sizeof(line), "IP %s", ipBuffer);
  } else {
    snprintf(line, sizeof(line), "WIFI OFF CH %u", WiFi.channel());
  }
  writeOledLine(2, line);

  if (gBootDerekTestRunning) {
    snprintf(
        line,
        sizeof(line),
        "TEST C%u D%u %s",
        gClusters[gBootDerekTestClusterCursor].clusterId,
        gBootDerekTestDerekCursor + 1,
        bootDerekTestPhaseName());
  } else {
    snprintf(line, sizeof(line), "TEST %s", gBootDerekTestDone ? "DONE" : "WAIT");
  }
  writeOledLine(3, line);

  if (gOscMessagesAccepted > 0) {
    snprintf(line, sizeof(line), "OSC C%u CH%u V%u", gLastOscClusterId, gLastOscChannel, gLastOscValue);
  } else if (gOscPacketsReceived > 0) {
    snprintf(line, sizeof(line), "OSC RX%lu %s", static_cast<unsigned long>(gOscPacketsReceived), gLastOscRejectReason);
  } else {
    snprintf(line, sizeof(line), "OSC WAIT %u", kOscListenPort);
  }
  writeOledLine(4, line);

  for (uint8_t lineIndex = 0; lineIndex < 3; ++lineIndex) {
    const uint8_t clusterIndex = lineIndex;
    if (clusterIndex < kMaxClusters && gClusters[clusterIndex].occupied) {
      snprintf(
          line,
          sizeof(line),
          "C%u D%u %s P%u",
          gClusters[clusterIndex].clusterId,
          gClusters[clusterIndex].lastActiveDerricks,
          gClusters[clusterIndex].online ? "ON" : "OFF",
          gClusters[clusterIndex].lastPirBits);
    } else {
      snprintf(line, sizeof(line), "C- WAITING");
    }
    writeOledLine(5 + lineIndex, line);
  }
}

uint16_t readOscPaddedString(const uint8_t* data, size_t len, size_t offset, char* output, size_t outputLen) {
  if (offset >= len || outputLen == 0) {
    return 0;
  }

  size_t cursor = offset;
  size_t outCursor = 0;
  while (cursor < len && data[cursor] != '\0') {
    if (outCursor < outputLen - 1) {
      output[outCursor++] = static_cast<char>(data[cursor]);
    }
    ++cursor;
  }

  if (cursor >= len) {
    output[0] = '\0';
    return 0;
  }

  output[outCursor] = '\0';
  ++cursor;
  while ((cursor % 4) != 0) {
    ++cursor;
  }
  return static_cast<uint16_t>(cursor);
}

uint32_t readOscUint32(const uint8_t* data, size_t offset) {
  return (static_cast<uint32_t>(data[offset]) << 24) |
         (static_cast<uint32_t>(data[offset + 1]) << 16) |
         (static_cast<uint32_t>(data[offset + 2]) << 8) |
         static_cast<uint32_t>(data[offset + 3]);
}

uint8_t valueToByte(float value) {
  if (value <= 1.0f) {
    return static_cast<uint8_t>(constrain(lroundf(value * 255.0f), 0, 255));
  }
  return static_cast<uint8_t>(constrain(lroundf(value), 0, 255));
}

uint8_t dmxToPan(uint8_t value) {
  return map(value, 0, 255, 10, 170);
}

uint8_t dmxToLift(uint8_t value) {
  return map(value, 0, 255, 0, 170);
}

MovementSpeed dmxToMovementSpeed(uint8_t value) {
  if (value < 85) {
    return kMovementSpeedFast;
  }
  if (value < 170) {
    return kMovementSpeedMedium;
  }
  return kMovementSpeedSlow;
}

bool parseMovementSpeedName(const char* speedName, MovementSpeed& movementSpeed) {
  if (strcmp(speedName, "fast") == 0 || strcmp(speedName, "Fast") == 0) {
    movementSpeed = kMovementSpeedFast;
    return true;
  }
  if (strcmp(speedName, "medium") == 0 || strcmp(speedName, "Medium") == 0) {
    movementSpeed = kMovementSpeedMedium;
    return true;
  }
  if (strcmp(speedName, "slow") == 0 || strcmp(speedName, "Slow") == 0) {
    movementSpeed = kMovementSpeedSlow;
    return true;
  }
  return false;
}

void initializeDefaultCommand(ClusterCommandPacket& command, uint8_t clusterId) {
  memset(&command, 0, sizeof(command));
  command.magic = kProtocolMagic;
  command.version = kProtocolVersion;
  command.kind = kMessageCommand;
  command.clusterId = clusterId;
  command.activeDerricks = kMaxDerricks;
  command.movementSpeed = kMovementSpeedFast;
  command.flags = kCommandFlagApplyOutputs;

  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    command.derricks[i].pan = 90;
    command.derricks[i].lift = 0;
  }
}

void setAudioTrackByte(uint16_t& track, bool highByte, uint8_t value) {
  if (highByte) {
    track = static_cast<uint16_t>((track & 0x00FF) | (static_cast<uint16_t>(value) << 8));
  } else {
    track = static_cast<uint16_t>((track & 0xFF00) | value);
  }
}

void triggerAudioTrack(ClusterRuntime& cluster, bool playerA) {
  ClusterCommandPacket& packet = cluster.desiredCommand;
  const uint16_t track = playerA ? cluster.audioTrackASetting : cluster.audioTrackBSetting;
  if (track == 0) {
    return;
  }

  if (playerA) {
    packet.audioTrackA = track;
    packet.flags |= kCommandFlagAudioA;
  } else {
    packet.audioTrackB = track;
    packet.flags |= kCommandFlagAudioB;
  }
  packet.flags |= kCommandFlagRequestStatus;
}

void applyClusterControllerChannel(ClusterRuntime& cluster, uint16_t channel, uint8_t value) {
  ClusterCommandPacket& packet = cluster.desiredCommand;
  const bool high = value >= 128;

  switch (channel) {
    case kControllerActiveDerricksChannel:
      packet.activeDerricks = value > kMaxDerricks ? kMaxDerricks : value;
      packet.flags |= kCommandFlagRequestStatus;
      break;
    case kControllerApplyOutputsChannel:
      if (high) {
        packet.flags |= kCommandFlagApplyOutputs;
      } else {
        packet.flags &= ~kCommandFlagApplyOutputs;
      }
      packet.flags |= kCommandFlagRequestStatus;
      break;
    case kControllerEmergencyHideChannel:
      if (high) {
        packet.flags |= kCommandFlagEmergencyHide | kCommandFlagRequestStatus;
      }
      break;
    case kControllerRequestStatusChannel:
      if (high) {
        packet.flags |= kCommandFlagRequestStatus;
      }
      break;
    case kControllerDiscoveryChannel:
      if (high) {
        packet.flags |= kCommandFlagDiscovery | kCommandFlagRequestStatus;
      }
      break;
    case kControllerAudioATrackMsbChannel:
      setAudioTrackByte(cluster.audioTrackASetting, true, value);
      break;
    case kControllerAudioATrackLsbChannel:
      setAudioTrackByte(cluster.audioTrackASetting, false, value);
      break;
    case kControllerAudioATriggerChannel:
      if (high && !cluster.audioATriggerHigh) {
        triggerAudioTrack(cluster, true);
      }
      cluster.audioATriggerHigh = high;
      break;
    case kControllerAudioBTrackMsbChannel:
      setAudioTrackByte(cluster.audioTrackBSetting, true, value);
      break;
    case kControllerAudioBTrackLsbChannel:
      setAudioTrackByte(cluster.audioTrackBSetting, false, value);
      break;
    case kControllerAudioBTriggerChannel:
      if (high && !cluster.audioBTriggerHigh) {
        triggerAudioTrack(cluster, false);
      }
      cluster.audioBTriggerHigh = high;
      break;
    case kControllerMovementSpeedChannel:
      cluster.movementSpeed = dmxToMovementSpeed(value);
      packet.movementSpeed = cluster.movementSpeed;
      packet.flags |= kCommandFlagRequestStatus;
      break;
    case kControllerPirCenterChannel:
    case kControllerPirRightChannel:
      // PIR channels are feedback/status placeholders in the QLC fixture.
      break;
  }
}

void applyDerekChannel(ClusterCommandPacket& packet, uint16_t channel, uint8_t value) {
  if (channel == 0) {
    return;
  }

  packet.flags |= kCommandFlagRequestStatus;

  if (channel < kDerekChannelsStart || channel > kChannelsPerCluster) {
    return;
  }

  const uint16_t zeroBased = channel - kDerekChannelsStart;
  const uint8_t derekIndex = zeroBased / kChannelsPerDerek;
  const uint8_t channelOffset = zeroBased % kChannelsPerDerek;
  DerekCommand& derek = packet.derricks[derekIndex];
  packet.flags |= kCommandFlagApplyOutputs;

  switch (channelOffset) {
    case 0:
      derek.pan = dmxToPan(value);
      break;
    case 1:
      derek.lift = dmxToLift(value);
      break;
    case 2:
      derek.eyeColor.r = value;
      break;
    case 3:
      derek.eyeColor.g = value;
      break;
    case 4:
      derek.eyeColor.b = value;
      break;
    case 5:
      derek.canColor.r = value;
      break;
    case 6:
      derek.canColor.g = value;
      break;
    case 7:
      derek.canColor.b = value;
      break;
    case 8:
      derek.exteriorColor.r = value;
      break;
    case 9:
      derek.exteriorColor.g = value;
      break;
    case 10:
      derek.exteriorColor.b = value;
      break;
  }
}

void applyDmxChannelToCluster(ClusterRuntime& cluster, uint16_t channel, uint8_t value) {
  if (channel == 0 || channel > kChannelsPerCluster) {
    return;
  }

  if (channel <= kClusterControllerChannels) {
    applyClusterControllerChannel(cluster, channel, value);
    return;
  }

  applyDerekChannel(cluster.desiredCommand, channel, value);
}

int8_t findClusterIndexByMac(const uint8_t* macAddress) {
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied && memcmp(gClusters[i].mac, macAddress, 6) == 0) {
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

int8_t findClusterIndexById(uint8_t clusterId) {
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied && gClusters[i].clusterId == clusterId) {
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

int8_t reserveClusterSlot(const uint8_t* macAddress, uint8_t clusterId) {
  const int8_t existingByMac = findClusterIndexByMac(macAddress);
  if (existingByMac >= 0) {
    return existingByMac;
  }

  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (!gClusters[i].occupied) {
      gClusters[i].occupied = true;
      gClusters[i].online = true;
      gClusters[i].clusterId = clusterId;
      gClusters[i].lastActiveDerricks = kMaxDerricks;
      gClusters[i].movementSpeed = kMovementSpeedFast;
      memcpy(gClusters[i].mac, macAddress, 6);
      initializeDefaultCommand(gClusters[i].desiredCommand, clusterId);
      addEspNowPeer(macAddress);
      return static_cast<int8_t>(i);
    }
  }

  return -1;
}

void updateClusterFromStatus(const uint8_t* macAddress, const ClusterStatusPacket& status, uint32_t nowMs) {
  int8_t clusterIndex = reserveClusterSlot(macAddress, status.clusterId);
  if (clusterIndex < 0) {
    return;
  }

  ClusterRuntime& cluster = gClusters[clusterIndex];
  cluster.online = true;
  cluster.clusterId = status.clusterId;
  cluster.lastSequence = status.sequence;
  cluster.lastActiveDerricks = status.activeDerricks > kMaxDerricks ? kMaxDerricks : status.activeDerricks;
  cluster.lastPirBits = status.pirStateBits;
  cluster.lastHealthFlags = status.healthFlags;
  cluster.lastSeenAtMs = nowMs;
  cluster.lastStatusUptimeMs = status.uptimeMs;
  cluster.desiredCommand.clusterId = status.clusterId;
  cluster.desiredCommand.activeDerricks = cluster.lastActiveDerricks;
}

void printClusterSummary(const ClusterRuntime& cluster) {
  char macBuffer[18] = {};
  formatMac(cluster.mac, macBuffer, sizeof(macBuffer));

  Serial.print("Cluster ");
  Serial.print(cluster.clusterId);
  Serial.print(" [");
  Serial.print(macBuffer);
  Serial.print("] online=");
  Serial.print(cluster.online ? "yes" : "no");
  Serial.print(" pir=");
  Serial.print(cluster.lastPirBits, BIN);
  Serial.print(" health=0x");
  Serial.print(cluster.lastHealthFlags, HEX);
  Serial.print(" uptimeMs=");
  Serial.println(cluster.lastStatusUptimeMs);
}

void renderStatusPage(uint32_t nowMs) {
  writeOledStatusPage(nowMs);

  if (!kEnableSerialLogs) {
    return;
  }

  Serial.println("--- MCU Status ---");
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied) {
      printClusterSummary(gClusters[i]);
    }
  }
}

void initializeDisplayHardware() {
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  gOledReady = initializeOled();
  if (gOledReady) {
    clearOled();
    char line[22] = {};
    snprintf(
        line,
        sizeof(line),
        "MCU V%u.%u.%u",
        kSoftwareVersionMajor,
        kSoftwareVersionMinor,
        kSoftwareVersionRevision);
    writeOledLine(0, line);
    writeOledLine(1, "OLED READY");
    writeOledLine(2, "DISCOVERY ON");
  }
}

void updateButtonInputs() {
  // TODO: Replace with actual page navigation once button pins and OLED are known.
  if (kButtonNextPin >= 0) {
    pinMode(kButtonNextPin, INPUT_PULLUP);
  }
  if (kButtonPrevPin >= 0) {
    pinMode(kButtonPrevPin, INPUT_PULLUP);
  }
}

void markOfflineClusters(uint32_t nowMs) {
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (!gClusters[i].occupied) {
      continue;
    }

    gClusters[i].online = (nowMs - gClusters[i].lastSeenAtMs) <= kClusterOfflineMs;
  }
}

void setAllDerekNeutral(ClusterCommandPacket& command) {
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    command.derricks[i].pan = 90;
    command.derricks[i].lift = 0;
  }
}

void applyBootDerekTestPose(ClusterRuntime& cluster, uint8_t derekIndex, BootDerekTestPhase phase) {
  ClusterCommandPacket& command = cluster.desiredCommand;
  setAllDerekNeutral(command);
  command.flags = kCommandFlagApplyOutputs | kCommandFlagRequestStatus;
  command.activeDerricks = kMaxDerricks;

  if (derekIndex >= kMaxDerricks) {
    return;
  }

  DerekCommand& derek = command.derricks[derekIndex];
  derek.eyeColor = {40, 40, 40};
  derek.canColor = {0, 0, 0};
  derek.exteriorColor = {0, 0, 0};

  switch (phase) {
    case kBootDerekTestLift:
      derek.pan = 90;
      derek.lift = 170;
      break;
    case kBootDerekTestLeft:
      derek.pan = 10;
      derek.lift = 170;
      break;
    case kBootDerekTestRight:
      derek.pan = 170;
      derek.lift = 170;
      break;
    case kBootDerekTestCenter:
      derek.pan = 90;
      derek.lift = 170;
      break;
    case kBootDerekTestLower:
      derek.pan = 90;
      derek.lift = 0;
      derek.eyeColor = {0, 0, 0};
      break;
    default:
      break;
  }
}

int8_t findNextOnlineClusterIndex(uint8_t startIndex) {
  for (uint8_t i = startIndex; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied && gClusters[i].online && gClusters[i].lastActiveDerricks > 0) {
      return static_cast<int8_t>(i);
    }
  }
  return -1;
}

void finishBootDerekTest() {
  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied) {
      setAllDerekNeutral(gClusters[i].desiredCommand);
      gClusters[i].desiredCommand.flags = kCommandFlagApplyOutputs | kCommandFlagRequestStatus;
    }
  }
  gBootDerekTestRunning = false;
  gBootDerekTestDone = true;
  Serial.println("Boot Derek test complete.");
}

void advanceBootDerekTestCursor() {
  if (gBootDerekTestPhase < kBootDerekTestLower) {
    gBootDerekTestPhase = static_cast<BootDerekTestPhase>(gBootDerekTestPhase + 1);
    return;
  }

  gBootDerekTestPhase = kBootDerekTestLift;
  ++gBootDerekTestDerekCursor;
  if (gBootDerekTestDerekCursor < gClusters[gBootDerekTestClusterCursor].lastActiveDerricks) {
    return;
  }

  gBootDerekTestDerekCursor = 0;
  const int8_t nextCluster = findNextOnlineClusterIndex(gBootDerekTestClusterCursor + 1);
  if (nextCluster >= 0) {
    gBootDerekTestClusterCursor = static_cast<uint8_t>(nextCluster);
    return;
  }

  finishBootDerekTest();
}

void runBootDerekTest(uint32_t nowMs) {
  if (gBootDerekTestDone) {
    return;
  }

  if (!gBootDerekTestRunning) {
    if ((nowMs - gBootStartedAtMs) < kBootDerekTestWaitMs) {
      return;
    }

    const int8_t firstCluster = findNextOnlineClusterIndex(0);
    if (firstCluster < 0) {
      finishBootDerekTest();
      Serial.println("Boot Derek test skipped; no registered clusters.");
      return;
    }

    gBootDerekTestRunning = true;
    gBootDerekTestClusterCursor = static_cast<uint8_t>(firstCluster);
    gBootDerekTestDerekCursor = 0;
    gBootDerekTestPhase = kBootDerekTestLift;
    gLastBootDerekTestStepAtMs = 0;
    Serial.println("Boot Derek test started.");
  }

  if ((nowMs - gLastBootDerekTestStepAtMs) < kBootDerekTestStepMs) {
    return;
  }

  gLastBootDerekTestStepAtMs = nowMs;
  if (!gClusters[gBootDerekTestClusterCursor].occupied || !gClusters[gBootDerekTestClusterCursor].online ||
      gClusters[gBootDerekTestClusterCursor].lastActiveDerricks == 0) {
    const int8_t nextCluster = findNextOnlineClusterIndex(gBootDerekTestClusterCursor + 1);
    if (nextCluster < 0) {
      finishBootDerekTest();
      return;
    }
    gBootDerekTestClusterCursor = static_cast<uint8_t>(nextCluster);
    gBootDerekTestDerekCursor = 0;
    gBootDerekTestPhase = kBootDerekTestLift;
  }

  ClusterRuntime& cluster = gClusters[gBootDerekTestClusterCursor];
  applyBootDerekTestPose(cluster, gBootDerekTestDerekCursor, gBootDerekTestPhase);

  Serial.print("Boot test C");
  Serial.print(cluster.clusterId);
  Serial.print(" Derek ");
  Serial.print(gBootDerekTestDerekCursor + 1);
  Serial.print(" ");
  Serial.println(bootDerekTestPhaseName());

  advanceBootDerekTestCursor();
}

void printRadioDebugSummary() {
  Serial.print("ESP-NOW queued=");
  Serial.print(gEspNowPacketsQueued);
  Serial.print(" queueFail=");
  Serial.print(gEspNowQueueFailures);
  Serial.print(" sendOk=");
  Serial.print(gEspNowSendSuccesses);
  Serial.print(" sendFail=");
  Serial.print(gEspNowSendFailures);
  Serial.print(" last C");
  Serial.print(gLastEspNowClusterId);
  Serial.print(" seq=");
  Serial.print(gLastEspNowSequence);
  Serial.print(" flags=0x");
  Serial.print(gLastEspNowFlags, HEX);
  Serial.print(" queueResult=");
  Serial.print(gLastEspNowQueueResult);
  Serial.print(" cbStatus=");
  Serial.println(gLastEspNowSendStatus);
}

void maybePrintRadioDebugSummary(uint32_t nowMs) {
  if (!kEnableSerialLogs || !kEnableRadioDebugLogs) {
    return;
  }

  if ((nowMs - gLastRadioDebugLogAtMs) < kRadioDebugLogIntervalMs) {
    return;
  }

  gLastRadioDebugLogAtMs = nowMs;
  printRadioDebugSummary();
}

void sendCommandToCluster(ClusterRuntime& cluster) {
  cluster.desiredCommand.magic = kProtocolMagic;
  cluster.desiredCommand.version = kProtocolVersion;
  cluster.desiredCommand.kind = kMessageCommand;
  cluster.desiredCommand.sequence = gSequenceCounter++;
  cluster.desiredCommand.movementSpeed = cluster.movementSpeed;

  const uint16_t sentSequence = cluster.desiredCommand.sequence;
  const uint8_t sentFlags = cluster.desiredCommand.flags;
  const esp_err_t sendResult = esp_now_send(
      cluster.mac,
      reinterpret_cast<const uint8_t*>(&cluster.desiredCommand),
      sizeof(cluster.desiredCommand));

  gLastEspNowQueuedAtMs = millis();
  gLastEspNowClusterId = cluster.clusterId;
  gLastEspNowSequence = sentSequence;
  gLastEspNowFlags = sentFlags;
  gLastEspNowQueueResult = static_cast<int>(sendResult);
  if (sendResult == ESP_OK) {
    ++gEspNowPacketsQueued;
  } else {
    ++gEspNowQueueFailures;
    if (kEnableSerialLogs) {
      Serial.print("ESP-NOW queue failed C");
      Serial.print(cluster.clusterId);
      Serial.print(" seq=");
      Serial.print(sentSequence);
      Serial.print(" result=");
      Serial.println(gLastEspNowQueueResult);
    }
  }
  maybePrintRadioDebugSummary(gLastEspNowQueuedAtMs);

  cluster.desiredCommand.flags &= kCommandFlagApplyOutputs;
  cluster.desiredCommand.audioTrackA = 0;
  cluster.desiredCommand.audioTrackB = 0;
}

void initializeWifiAndOsc() {
  WiFi.mode(WIFI_STA);

  if (strlen(kWifiSsid) > 0) {
    WiFi.begin(kWifiSsid, kWifiPassword);
    const uint32_t startedAtMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startedAtMs) < 8000) {
      delay(50);
    }
  } else {
    WiFi.disconnect(false, true);
  }

  gOscUdp.begin(kOscListenPort);

  if (gOledReady) {
    if (WiFi.status() == WL_CONNECTED) {
      char ipBuffer[16] = {};
      formatIpAddress(WiFi.localIP(), ipBuffer, sizeof(ipBuffer));
      char line[22] = {};
      snprintf(line, sizeof(line), "IP %s", ipBuffer);
      writeOledLine(1, "WIFI CONNECTED");
      writeOledLine(2, line);
    } else {
      writeOledLine(1, "WIFI NOT CONNECTED");
      writeOledLine(2, "NO IP ADDRESS");
    }
  }

  if (!kEnableSerialLogs) {
    return;
  }

  Serial.print("Wi-Fi MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Wi-Fi channel: ");
  Serial.println(WiFi.channel());
  Serial.print("OSC listen port: ");
  Serial.println(kOscListenPort);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi connected.");
    Serial.print("Wi-Fi IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi is not connected; ESP-NOW and serial tests are still available.");
  }
}

void broadcastDiscovery() {
  ClusterCommandPacket discovery = {};
  discovery.magic = kProtocolMagic;
  discovery.version = kProtocolVersion;
  discovery.kind = kMessageCommand;
  discovery.clusterId = 0;
  discovery.sequence = gSequenceCounter++;
  discovery.flags = kCommandFlagDiscovery | kCommandFlagRequestStatus;
  discovery.activeDerricks = kMaxDerricks;

  esp_now_send(kBroadcastMac, reinterpret_cast<const uint8_t*>(&discovery), sizeof(discovery));
}

void sendScheduledCommands(uint32_t nowMs) {
  if ((nowMs - gLastCommandAtMs) < kCommandIntervalMs) {
    return;
  }

  gLastCommandAtMs = nowMs;

  for (uint8_t i = 0; i < kMaxClusters; ++i) {
    if (gClusters[i].occupied && gClusters[i].online) {
      sendCommandToCluster(gClusters[i]);
    }
  }
}

void handlePirEvent(const ClusterRuntime& cluster, uint8_t priorPirBits, uint8_t newPirBits) {
  if (priorPirBits == newPirBits) {
    return;
  }

  Serial.print("PIR change from cluster ");
  Serial.print(cluster.clusterId);
  Serial.print(": ");
  Serial.print(priorPirBits, BIN);
  Serial.print(" -> ");
  Serial.println(newPirBits, BIN);
  sendOscStatus(cluster);
}

void handleStatusPacket(const uint8_t* macAddress, const ClusterStatusPacket& status, uint32_t nowMs) {
  int8_t clusterIndex = reserveClusterSlot(macAddress, status.clusterId);
  if (clusterIndex < 0) {
    return;
  }

  const uint8_t previousPirBits = gClusters[clusterIndex].lastPirBits;
  updateClusterFromStatus(macAddress, status, nowMs);
  handlePirEvent(gClusters[clusterIndex], previousPirBits, status.pirStateBits);
}

bool parseOscDmxAddress(const char* address, uint8_t& clusterId, uint16_t& channel) {
  int universe = 0;
  int dmxChannel = 0;

  // QLC+ OSC output emits zero-based paths shaped like /<universe-1>/dmx/<channel-1>.
  // QLC Universe 1 / DMX channel 1 therefore arrives as /0/dmx/0.
  if (sscanf(address, "/%d/dmx/%d", &universe, &dmxChannel) == 2) {
    if (universe >= 0 && universe < kMaxClusters && dmxChannel >= 0) {
      clusterId = static_cast<uint8_t>(universe + 1);
      channel = static_cast<uint16_t>(dmxChannel + 1);
      return true;
    }
  }

  // Friendly direct test path for tools like Protokol or oscsend:
  // /derek/cluster/1/channel/1 255
  if (sscanf(address, "/derek/cluster/%d/channel/%d", &universe, &dmxChannel) == 2) {
    if (universe >= 1 && universe <= kMaxClusters && dmxChannel >= 1) {
      clusterId = static_cast<uint8_t>(universe);
      channel = static_cast<uint16_t>(dmxChannel);
      return true;
    }
  }

  return false;
}

bool parseOscValue(const uint8_t* data, size_t len, size_t offset, const char* typeTags, uint8_t& value) {
  if (typeTags[0] != ',' || typeTags[1] == '\0') {
    return false;
  }

  switch (typeTags[1]) {
    case 'i':
      if (offset + 4 > len) {
        return false;
      }
      value = static_cast<uint8_t>(constrain(static_cast<int32_t>(readOscUint32(data, offset)), 0, 255));
      return true;
    case 'f': {
      if (offset + 4 > len) {
        return false;
      }
      const uint32_t raw = readOscUint32(data, offset);
      float floatValue = 0.0f;
      memcpy(&floatValue, &raw, sizeof(floatValue));
      value = valueToByte(floatValue);
      return true;
    }
    case 'T':
      value = 255;
      return true;
    case 'F':
      value = 0;
      return true;
  }

  return false;
}

void sendOscStatus(const ClusterRuntime& cluster) {
  if (gLastOscRemotePort == 0) {
    return;
  }

  char message[96] = {};
  snprintf(
      message,
      sizeof(message),
      "/derek/cluster/%u/status online=%u pir=%u health=0x%04X",
      cluster.clusterId,
      cluster.online ? 1 : 0,
      cluster.lastPirBits,
      cluster.lastHealthFlags);

  gOscUdp.beginPacket(gLastOscRemoteIp, kOscStatusPort);
  gOscUdp.print(message);
  gOscUdp.endPacket();
}

void recordOscReject(const char* reason) {
  ++gOscMessagesRejected;
  strncpy(gLastOscRejectReason, reason, sizeof(gLastOscRejectReason) - 1);
  gLastOscRejectReason[sizeof(gLastOscRejectReason) - 1] = '\0';

  if (!kEnableSerialLogs || !kEnableOscDebugLogs) {
    return;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - gLastOscDebugLogAtMs) >= kOscDebugLogIntervalMs) {
    gLastOscDebugLogAtMs = nowMs;
    Serial.print("OSC rejected: ");
    Serial.print(gLastOscRejectReason);
    Serial.print(" packets=");
    Serial.print(gOscPacketsReceived);
    Serial.print(" accepted=");
    Serial.print(gOscMessagesAccepted);
    Serial.print(" rejected=");
    Serial.println(gOscMessagesRejected);
  }
}

void recordOscAccepted(const char* address, uint8_t clusterId, uint16_t channel, uint8_t value) {
  ++gOscMessagesAccepted;
  gLastOscAcceptedAtMs = millis();
  gLastOscClusterId = clusterId;
  gLastOscChannel = channel;
  gLastOscValue = value;
  strncpy(gLastOscAddress, address, sizeof(gLastOscAddress) - 1);
  gLastOscAddress[sizeof(gLastOscAddress) - 1] = '\0';
  strncpy(gLastOscRejectReason, "none", sizeof(gLastOscRejectReason) - 1);
  gLastOscRejectReason[sizeof(gLastOscRejectReason) - 1] = '\0';

  if (!kEnableSerialLogs || !kEnableOscDebugLogs) {
    return;
  }

  const uint32_t nowMs = millis();
  if ((nowMs - gLastOscDebugLogAtMs) >= kOscDebugLogIntervalMs) {
    gLastOscDebugLogAtMs = nowMs;
    Serial.print("OSC accepted ");
    Serial.print(address);
    Serial.print(" -> cluster ");
    Serial.print(clusterId);
    Serial.print(" channel ");
    Serial.print(channel);
    Serial.print(" value ");
    Serial.print(value);
    Serial.print(" from ");
    Serial.print(gLastOscRemoteIp);
    Serial.print(":");
    Serial.println(gLastOscRemotePort);
  }
}

void handleOscMessage(const uint8_t* data, size_t len) {
  char address[64] = {};
  char typeTags[16] = {};
  uint8_t clusterId = 0;
  uint16_t channel = 0;
  uint8_t value = 0;

  size_t offset = readOscPaddedString(data, len, 0, address, sizeof(address));
  if (offset == 0) {
    recordOscReject("bad-address");
    return;
  }

  offset = readOscPaddedString(data, len, offset, typeTags, sizeof(typeTags));
  if (offset == 0) {
    recordOscReject("bad-typetag");
    return;
  }
  if (!parseOscDmxAddress(address, clusterId, channel)) {
    recordOscReject("bad-path");
    return;
  }
  if (!parseOscValue(data, len, offset, typeTags, value)) {
    recordOscReject("bad-value");
    return;
  }

  const int8_t clusterIndex = findClusterIndexById(clusterId);
  if (clusterIndex < 0) {
    if (kEnableSerialLogs) {
      Serial.print("OSC for undiscovered cluster ");
      Serial.println(clusterId);
    }
    recordOscReject("no-cluster");
    return;
  }

  applyDmxChannelToCluster(gClusters[clusterIndex], channel, value);
  recordOscAccepted(address, clusterId, channel, value);
}

void handleOscPacket(const uint8_t* data, size_t len) {
  if (len < 4) {
    recordOscReject("short-packet");
    return;
  }

  if (len >= 16 && memcmp(data, "#bundle", 7) == 0) {
    size_t offset = 16;
    while (offset + 4 <= len) {
      const uint32_t elementLen = readOscUint32(data, offset);
      offset += 4;
      if (elementLen == 0 || offset + elementLen > len) {
        recordOscReject("bad-bundle");
        return;
      }
      handleOscMessage(data + offset, elementLen);
      offset += elementLen;
    }
    return;
  }

  handleOscMessage(data, len);
}

void updateOscInput() {
  const int packetSize = gOscUdp.parsePacket();
  if (packetSize <= 0) {
    return;
  }

  uint8_t packetBuffer[kOscPacketBufferSize] = {};
  const size_t bytesRead = gOscUdp.read(packetBuffer, min(packetSize, static_cast<int>(sizeof(packetBuffer))));
  gLastOscRemoteIp = gOscUdp.remoteIP();
  gLastOscRemotePort = gOscUdp.remotePort();
  ++gOscPacketsReceived;
  gLastOscPacketAtMs = millis();
  if (packetSize > static_cast<int>(sizeof(packetBuffer))) {
    recordOscReject("packet-too-large");
    return;
  }
  handleOscPacket(packetBuffer, bytesRead);
}

void parseSerialCommand(char* line) {
  char command[16] = {};
  int clusterId = 0;
  int derrickIndex = 0;
  int a = 0;
  int b = 0;
  int c = 0;

  if (sscanf(line, "%15s", command) != 1) {
    return;
  }

  if (strcmp(command, "list") == 0) {
    renderStatusPage(millis());
    return;
  }

  if (strcmp(command, "osc") == 0) {
    Serial.print("OSC packets=");
    Serial.print(gOscPacketsReceived);
    Serial.print(" accepted=");
    Serial.print(gOscMessagesAccepted);
    Serial.print(" rejected=");
    Serial.print(gOscMessagesRejected);
    Serial.print(" last=");
    Serial.print(gLastOscAddress);
    Serial.print(" C");
    Serial.print(gLastOscClusterId);
    Serial.print(" CH");
    Serial.print(gLastOscChannel);
    Serial.print(" V");
    Serial.print(gLastOscValue);
    Serial.print(" reject=");
    Serial.print(gLastOscRejectReason);
    Serial.print(" remote=");
    Serial.print(gLastOscRemoteIp);
    Serial.print(":");
    Serial.println(gLastOscRemotePort);
    return;
  }

  if (strcmp(command, "radio") == 0) {
    printRadioDebugSummary();
    return;
  }

  if (strcmp(command, "speed") == 0) {
    char speedName[8] = {};
    MovementSpeed movementSpeed = kMovementSpeedFast;
    if (sscanf(line, "%15s %d %7s", command, &clusterId, speedName) == 3 &&
        parseMovementSpeedName(speedName, movementSpeed)) {
      const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
      if (clusterIndex >= 0) {
        gClusters[clusterIndex].movementSpeed = movementSpeed;
        gClusters[clusterIndex].desiredCommand.movementSpeed = movementSpeed;
        gClusters[clusterIndex].desiredCommand.flags |= kCommandFlagRequestStatus;
      }
    }
    return;
  }

  if (strcmp(command, "dmx") == 0 &&
      sscanf(line, "%15s %d %d %d", command, &clusterId, &a, &b) == 4) {
    const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
    if (clusterIndex >= 0) {
      applyDmxChannelToCluster(
          gClusters[clusterIndex],
          static_cast<uint16_t>(constrain(a, 1, 512)),
          static_cast<uint8_t>(constrain(b, 0, 255)));
    }
    return;
  }

  if (strcmp(command, "audio") == 0 &&
      sscanf(line, "%15s %d %d %d", command, &clusterId, &a, &b) == 4) {
    const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
    if (clusterIndex >= 0) {
      ClusterCommandPacket& packet = gClusters[clusterIndex].desiredCommand;
      if (a == 1) {
        packet.audioTrackA = static_cast<uint16_t>(constrain(b, 1, 3000));
        packet.flags |= kCommandFlagAudioA | kCommandFlagRequestStatus;
      } else if (a == 2) {
        packet.audioTrackB = static_cast<uint16_t>(constrain(b, 1, 3000));
        packet.flags |= kCommandFlagAudioB | kCommandFlagRequestStatus;
      }
    }
    return;
  }

  if (strcmp(command, "hide") == 0 && sscanf(line, "%15s %d", command, &clusterId) == 2) {
    const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
    if (clusterIndex >= 0) {
      gClusters[clusterIndex].desiredCommand.flags = kCommandFlagEmergencyHide | kCommandFlagRequestStatus;
    }
    return;
  }

  if (strcmp(command, "track") == 0 &&
      sscanf(line, "%15s %d %d %d %d", command, &clusterId, &derrickIndex, &a, &b) == 5) {
    const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
    if (clusterIndex >= 0 && derrickIndex >= 0 && derrickIndex < kMaxDerricks) {
      ClusterCommandPacket& packet = gClusters[clusterIndex].desiredCommand;
      packet.flags = kCommandFlagApplyOutputs | kCommandFlagRequestStatus;
      packet.derricks[derrickIndex].pan = constrain(a, 0, 180);
      packet.derricks[derrickIndex].lift = constrain(b, 0, 170);
    }
    return;
  }

  if (strcmp(command, "eyes") == 0 &&
      sscanf(line, "%15s %d %d %d %d %d", command, &clusterId, &derrickIndex, &a, &b, &c) == 6) {
    const int8_t clusterIndex = findClusterIndexById(static_cast<uint8_t>(clusterId));
    if (clusterIndex >= 0 && derrickIndex >= 0 && derrickIndex < kMaxDerricks) {
      ClusterCommandPacket& packet = gClusters[clusterIndex].desiredCommand;
      packet.flags = kCommandFlagApplyOutputs | kCommandFlagRequestStatus;
      packet.derricks[derrickIndex].eyeColor = {
          static_cast<uint8_t>(constrain(a, 0, 255)),
          static_cast<uint8_t>(constrain(b, 0, 255)),
          static_cast<uint8_t>(constrain(c, 0, 255))};
    }
    return;
  }

  Serial.println("Commands: list | osc | radio | speed <cluster> <fast|medium|slow> | hide <cluster> | track <cluster> <derrick 0-7> <pan> <lift> | eyes <cluster> <derrick 0-7> <r> <g> <b> | dmx <cluster> <channel> <value> | audio <cluster> <player 1-2> <track>");
}

void updateSerialConsole() {
  static char lineBuffer[96] = {};
  static uint8_t lineLength = 0;

  while (Serial.available() > 0) {
    const char next = static_cast<char>(Serial.read());
    if (next == '\r') {
      continue;
    }

    if (next == '\n') {
      lineBuffer[lineLength] = '\0';
      parseSerialCommand(lineBuffer);
      lineLength = 0;
      continue;
    }

    if (lineLength < sizeof(lineBuffer) - 1) {
      lineBuffer[lineLength++] = next;
    }
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onEspNowReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (info == nullptr || data == nullptr || len <= 0 || !isProtocolPacketValid(data, static_cast<size_t>(len))) {
    return;
  }

  const uint8_t* senderMac = info->src_addr;
#else
void onEspNowReceive(const uint8_t* senderMac, const uint8_t* data, int len) {
  if (senderMac == nullptr || data == nullptr || len <= 0 || !isProtocolPacketValid(data, static_cast<size_t>(len))) {
    return;
  }
#endif
  const uint8_t kind = data[3];
  if ((kind == kMessageRegister || kind == kMessageStatus) && static_cast<size_t>(len) == sizeof(ClusterStatusPacket)) {
    ClusterStatusPacket status = {};
    memcpy(&status, data, sizeof(status));
    handleStatusPacket(senderMac, status, millis());
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onEspNowSend(const wifi_tx_info_t* txInfo, esp_now_send_status_t status) {
  (void)txInfo;
#else
void onEspNowSend(const uint8_t* macAddr, esp_now_send_status_t status) {
  (void)macAddr;
#endif
  gLastEspNowCallbackAtMs = millis();
  gLastEspNowSendStatus = static_cast<int>(status);
  if (status == ESP_NOW_SEND_SUCCESS) {
    ++gEspNowSendSuccesses;
  } else {
    ++gEspNowSendFailures;
  }
}

bool initializeEspNow() {
  if (esp_now_init() != ESP_OK) {
    return false;
  }

  addEspNowPeer(kBroadcastMac);
  esp_now_register_recv_cb(onEspNowReceive);
  esp_now_register_send_cb(onEspNowSend);
  return true;
}

}  // namespace

void setup() {
  if (kEnableSerialLogs) {
    Serial.begin(115200);
  }

  memset(gClusters, 0, sizeof(gClusters));
  gBootStartedAtMs = millis();
  initializeDisplayHardware();
  updateButtonInputs();
  initializeWifiAndOsc();

  if (!initializeEspNow()) {
    Serial.println("ESP-NOW init failed.");
    return;
  }

  Serial.print("MCU gateway ready. Version ");
  Serial.print(kSoftwareVersionMajor);
  Serial.print(".");
  Serial.print(kSoftwareVersionMinor);
  Serial.print(".");
  Serial.println(kSoftwareVersionRevision);
  Serial.println("Serial console commands: list, osc, radio, speed, hide, track, eyes, dmx, audio");
}

void loop() {
  const uint32_t nowMs = millis();

  updateSerialConsole();
  updateOscInput();
  markOfflineClusters(nowMs);

  if ((nowMs - gLastDiscoveryAtMs) >= kDiscoveryIntervalMs) {
    broadcastDiscovery();
    gLastDiscoveryAtMs = nowMs;
  }

  runBootDerekTest(nowMs);
  sendScheduledCommands(nowMs);

  if ((nowMs - gLastStatusPageAtMs) >= kStatusPageIntervalMs) {
    renderStatusPage(nowMs);
    gLastStatusPageAtMs = nowMs;
    gCurrentPage = (gCurrentPage + 1) % 2;
  }
}
