#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_PWMServoDriver.h>
#include <DFPlayerMini_Fast.h>
#include <Preferences.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_now.h>

#ifndef ARDUINO_ARCH_ESP32
#error "This sketch targets ESP32-class boards."
#endif

namespace {

constexpr bool kEnableSerialLogs = true;
constexpr uint8_t kSoftwareVersionMajor = 1;
constexpr uint8_t kSoftwareVersionMinor = 5;
constexpr uint8_t kSoftwareVersionRevision = 0;
constexpr uint8_t kProtocolVersion = 1;
constexpr uint16_t kProtocolMagic = 0xD311;
constexpr uint8_t kDefaultClusterId = 0;
constexpr uint8_t kMaxDerricks = 8;
constexpr uint8_t kMaxClusters = 15;
constexpr uint8_t kBroadcastClusterId = 0xFF;
constexpr uint32_t kStatusIntervalMs = 250;
constexpr uint32_t kRegistrationIntervalMs = 1000;
constexpr uint32_t kCommandTimeoutMs = 1500;
constexpr uint32_t kMotionUpdateIntervalMs = 20;
constexpr uint32_t kServoSoftRestDelayMs = 1000;
constexpr uint32_t kI2cClockHz = 400000;
constexpr uint32_t kServoOutputServiceIntervalMs = 5;
constexpr uint8_t kServoDereksPerService = 1;
constexpr uint8_t kServoOutputRefreshPasses = 3;
constexpr uint8_t kCommandQueueDepth = 8;
constexpr uint8_t kMaxQueuedCommandsPerLoop = 4;
constexpr bool kRunSelfTestAtBoot = false;
constexpr uint32_t kSelfTestIntervalMs = 1000;
constexpr uint32_t kFullClusterTestStageMs = 2500;
constexpr uint16_t kSelfTestAudioTrack = 1;
constexpr uint8_t kPanStepPerTick = 2;
constexpr uint8_t kLiftStepPerTick = 2;
constexpr uint8_t kMediumPanStepPerTick = 8;
constexpr uint8_t kMediumLiftStepPerTick = 8;
constexpr size_t kSerialCommandBufferSize = 96;
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

constexpr int8_t kI2cSdaPin = 8;
constexpr int8_t kI2cSclPin = 9;
constexpr int8_t kClusterIdPins[4] = {4, 5, 6, 7};
constexpr int8_t kLedDataPin = 10;
constexpr int8_t kPirPins[3] = {11, 12, 13};
constexpr int8_t kPcaOePin = 14;
constexpr int8_t kDfPlayer2TxPin = 15;
constexpr int8_t kDfPlayer2RxPin = 16;
constexpr int8_t kDfPlayer1TxPin = 17;
constexpr int8_t kDfPlayer1RxPin = 18;
constexpr int8_t kServoPanChannels[kMaxDerricks] = {0, 2, 4, 6, 8, 10, 12, 14};
constexpr int8_t kServoLiftChannels[kMaxDerricks] = {1, 3, 5, 7, 9, 11, 13, 15};
constexpr uint8_t kPca9685Address = 0x40;
constexpr uint8_t kOledAddress = 0x3C;
constexpr uint16_t kServoMinPulseUs = 500;
constexpr uint16_t kServoMaxPulseUs = 2500;
constexpr uint16_t kServoPwmFrequencyHz = 50;
// Physical LED-chain order for every Derek: two spotlight pixels, one right
// eye pixel, then one left eye pixel.
constexpr uint8_t kSpotlightLedsPerDerek = 2;
constexpr uint8_t kRightEyeLedsPerDerek = 1;
constexpr uint8_t kLeftEyeLedsPerDerek = 1;
constexpr uint8_t kLedsPerDerek =
    kSpotlightLedsPerDerek + kRightEyeLedsPerDerek + kLeftEyeLedsPerDerek;
constexpr uint16_t kTotalLedCount = kMaxDerricks * kLedsPerDerek;
constexpr uint8_t kDerekTypeConfigVersion = 1;

enum CanType : uint8_t {
  kCanSmallest = 0,
  kCanSmall = 1,
  kCanMedium = 2,
  kCanTall = 3,
  kCanTallest = 4,
  kCanTypeCount = 5,
};

struct CanLiftProfile {
  uint8_t minPercent;
  uint8_t maxPercent;
};

constexpr uint8_t kLiftTravelMinAngle = 0;
constexpr uint8_t kLiftTravelMaxAngle = 170;
constexpr CanLiftProfile kDefaultCanLiftProfiles[kCanTypeCount] = {
    {0, 80},  // Smallest
    {0, 80},  // Small
    {0, 80},  // Medium
    {0, 80},  // Tall
    {0, 80},  // Tallest
};
constexpr CanType kDefaultDerekCanTypes[kMaxDerricks] = {
    kCanMedium,
    kCanMedium,
    kCanMedium,
    kCanMedium,
    kCanMedium,
    kCanMedium,
    kCanMedium,
    kCanMedium,
};

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

enum HealthFlags : uint16_t {
  kHealthServosReady = 1 << 0,
  kHealthLedsReady = 1 << 1,
  kHealthAudio1Ready = 1 << 2,
  kHealthAudio2Ready = 1 << 3,
  kHealthPirReady = 1 << 4,
  kHealthControllerLinked = 1 << 5,
  kHealthCommandStale = 1 << 6,
  kHealthServoPwmEnabled = 1 << 7,
};

enum SelfTestMode : uint8_t {
  kSelfTestModeOff = 0,
  kSelfTestModeSingle = 1,
  kSelfTestModeFull = 2,
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
  RgbColor spotlightColor;
  RgbColor rightEyeColor;
  RgbColor leftEyeColor;
};

struct DerekCalibration {
  uint8_t panMin;
  uint8_t panCenter;
  uint8_t panMax;
  uint8_t liftMin;
  uint8_t liftMax;
};

struct DerekState {
  uint8_t currentPan;
  uint8_t currentLift;
  uint8_t targetPan;
  uint8_t targetLift;
  DerekCommand output;
  DerekCalibration calibration;
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

struct QueuedCommand {
  ClusterCommandPacket packet;
  uint8_t senderMac[6];
};

DerekState gDerricks[kMaxDerricks];
CanLiftProfile gCanLiftProfiles[kCanTypeCount];
CanType gDerekCanTypes[kMaxDerricks];
uint8_t gClusterId = kDefaultClusterId;
uint8_t gActiveDerricks = kMaxDerricks;
uint8_t gPirStateBits = 0;
uint8_t gLastReportedPirBits = 0;
uint8_t gControllerMac[6] = {0};
bool gControllerMacKnown = false;
bool gServosReady = false;
bool gLedsReady = false;
bool gAudio1Ready = false;
bool gAudio2Ready = false;
bool gPirReady = false;
bool gServoPwmEnabled = false;
uint16_t gLastSequence = 0;
uint32_t gLastCommandAtMs = 0;
uint32_t gLastStatusAtMs = 0;
uint32_t gLastRegistrationAtMs = 0;
uint32_t gLastMotionUpdateAtMs = 0;
uint32_t gLastServoMotionAtMs = 0;
uint16_t gPendingAudioTrackA = 0;
uint16_t gPendingAudioTrackB = 0;
bool gPendingAudioA = false;
bool gPendingAudioB = false;
portMUX_TYPE gPendingCommandMux = portMUX_INITIALIZER_UNLOCKED;
QueuedCommand gCommandQueue[kCommandQueueDepth] = {};
volatile uint8_t gCommandQueueHead = 0;
volatile uint8_t gCommandQueueTail = 0;
volatile uint8_t gCommandQueueCount = 0;
uint32_t gCommandQueueOverflows = 0;
uint8_t gServoDirtyMask = 0;
uint8_t gServoRefreshPassesRemaining[kMaxDerricks] = {};
uint8_t gNextServoServiceIndex = 0;
uint32_t gLastServoOutputServiceAtMs = 0;
uint32_t gServoI2cWrites = 0;
uint32_t gServoI2cFailures = 0;
MovementSpeed gMovementSpeed = kMovementSpeedFast;
uint32_t gLastSelfTestAtMs = 0;
uint8_t gSelfTestStep = 0;
uint32_t gLastFullClusterTestAtMs = 0;
uint8_t gFullClusterTestStage = 0;
SelfTestMode gSelfTestMode = kRunSelfTestAtBoot ? kSelfTestModeFull : kSelfTestModeOff;
char gSerialCommandBuffer[kSerialCommandBufferSize] = {};
size_t gSerialCommandLength = 0;
bool gOledReady = false;
bool gOledRuntimeRefreshPending = false;
Adafruit_NeoPixel gLedStrip(kTotalLedCount, kLedDataPin, NEO_GRB + NEO_KHZ800);
Adafruit_PWMServoDriver gServoDriver(kPca9685Address, Wire);
HardwareSerial gDfPlayer1Serial(1);
HardwareSerial gDfPlayer2Serial(2);
DFPlayerMini_Fast gDfPlayer1;
DFPlayerMini_Fast gDfPlayer2;
Preferences gConfigPreferences;

void writeLedOutputs();
void writeOledTestOutput(uint8_t derrickIndex, uint8_t step);
void writeOledSequenceStage(uint8_t stage);
void applyFailsafeTargets();
void markServoOutputDirty(uint8_t derrickIndex);
void markActiveServoOutputsDirty();

void logLine(const char* message) {
  if (kEnableSerialLogs) {
    Serial.println(message);
  }
}

uint8_t clampU8(uint8_t value, uint8_t minimum, uint8_t maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
}

bool isCanTypeValid(CanType canType) {
  return static_cast<uint8_t>(canType) < kCanTypeCount;
}

CanLiftProfile sanitizeCanLiftProfile(CanLiftProfile profile) {
  profile.minPercent = clampU8(profile.minPercent, 0, 100);
  profile.maxPercent = clampU8(profile.maxPercent, 0, 100);
  if (profile.minPercent > profile.maxPercent) {
    const uint8_t swappedMin = profile.maxPercent;
    profile.maxPercent = profile.minPercent;
    profile.minPercent = swappedMin;
  }
  return profile;
}

void loadDefaultDerekTypeConfig() {
  for (uint8_t i = 0; i < kCanTypeCount; ++i) {
    gCanLiftProfiles[i] = kDefaultCanLiftProfiles[i];
  }
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    gDerekCanTypes[i] = kDefaultDerekCanTypes[i];
  }
}

void saveDerekTypeConfig() {
  // Serial configuration commands, entered at 115200 baud with newline:
  //   help
  //   config
  //   type <derek 1-8> <smallest|small|medium|tall|tallest>
  //   profile <smallest|small|medium|tall|tallest> <minPercent 0-100> <maxPercent 0-100>
  //   defaults
  // Successful type/profile/defaults commands call this method and persist to ESP32 NVS.
  if (!gConfigPreferences.begin("derek-types", false)) {
    logLine("Derek type config save failed.");
    return;
  }

  uint8_t canTypeBytes[kMaxDerricks];
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    canTypeBytes[i] = static_cast<uint8_t>(gDerekCanTypes[i]);
  }

  gConfigPreferences.putUChar("version", kDerekTypeConfigVersion);
  gConfigPreferences.putBytes("profiles", gCanLiftProfiles, sizeof(gCanLiftProfiles));
  gConfigPreferences.putBytes("assign", canTypeBytes, sizeof(canTypeBytes));
  gConfigPreferences.end();
}

void loadDerekTypeConfig() {
  loadDefaultDerekTypeConfig();

  if (!gConfigPreferences.begin("derek-types", true)) {
    logLine("Using default Derek type config.");
    saveDerekTypeConfig();
    return;
  }

  const uint8_t storedVersion = gConfigPreferences.getUChar("version", 0);
  const size_t profileBytes = gConfigPreferences.getBytesLength("profiles");
  const size_t assignmentBytes = gConfigPreferences.getBytesLength("assign");
  const bool hasStoredConfig =
      storedVersion == kDerekTypeConfigVersion && profileBytes == sizeof(gCanLiftProfiles) &&
      assignmentBytes == kMaxDerricks;

  if (hasStoredConfig) {
    gConfigPreferences.getBytes("profiles", gCanLiftProfiles, sizeof(gCanLiftProfiles));
    uint8_t canTypeBytes[kMaxDerricks] = {};
    gConfigPreferences.getBytes("assign", canTypeBytes, sizeof(canTypeBytes));
    for (uint8_t i = 0; i < kMaxDerricks; ++i) {
      const CanType storedType = static_cast<CanType>(canTypeBytes[i]);
      gDerekCanTypes[i] = isCanTypeValid(storedType) ? storedType : kDefaultDerekCanTypes[i];
    }
    for (uint8_t i = 0; i < kCanTypeCount; ++i) {
      gCanLiftProfiles[i] = sanitizeCanLiftProfile(gCanLiftProfiles[i]);
    }
  }

  gConfigPreferences.end();

  if (!hasStoredConfig) {
    logLine("Writing default Derek type config.");
    saveDerekTypeConfig();
  }
}

const char* canTypeName(CanType canType) {
  switch (canType) {
    case kCanSmallest:
      return "Smallest";
    case kCanSmall:
      return "Small";
    case kCanMedium:
      return "Medium";
    case kCanTall:
      return "Tall";
    case kCanTallest:
      return "Tallest";
    default:
      return "Unknown";
  }
}

bool parseCanType(const char* text, CanType& canType) {
  if (strcasecmp(text, "smallest") == 0 || strcmp(text, "0") == 0) {
    canType = kCanSmallest;
    return true;
  }
  if (strcasecmp(text, "small") == 0 || strcmp(text, "1") == 0) {
    canType = kCanSmall;
    return true;
  }
  if (strcasecmp(text, "medium") == 0 || strcmp(text, "2") == 0) {
    canType = kCanMedium;
    return true;
  }
  if (strcasecmp(text, "tall") == 0 || strcmp(text, "3") == 0) {
    canType = kCanTall;
    return true;
  }
  if (strcasecmp(text, "tallest") == 0 || strcmp(text, "4") == 0) {
    canType = kCanTallest;
    return true;
  }
  return false;
}

bool parsePercent(const char* text, uint8_t& percent) {
  char* end = nullptr;
  const long value = strtol(text, &end, 10);
  if (end == text || *end != '\0' || value < 0 || value > 100) {
    return false;
  }
  percent = static_cast<uint8_t>(value);
  return true;
}

void initializeClusterIdInputs() {
  for (uint8_t i = 0; i < 4; ++i) {
    pinMode(kClusterIdPins[i], INPUT_PULLUP);
  }

  uint8_t dipValue = 0;
  for (uint8_t i = 0; i < 4; ++i) {
    if (digitalRead(kClusterIdPins[i]) == LOW) {
      dipValue |= 1 << i;
    }
  }

  gClusterId = dipValue >= kMaxClusters ? kDefaultClusterId : dipValue;

  if (kEnableSerialLogs) {
    Serial.print("DIP Universe/Cluster ID: ");
    Serial.println(gClusterId);
  }
}

void printSerialConfig() {
  if (!kEnableSerialLogs) {
    return;
  }

  Serial.println("Derek type profiles:");
  for (uint8_t i = 0; i < kCanTypeCount; ++i) {
    Serial.print("  ");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(canTypeName(static_cast<CanType>(i)));
    Serial.print(": min=");
    Serial.print(gCanLiftProfiles[i].minPercent);
    Serial.print("% max=");
    Serial.print(gCanLiftProfiles[i].maxPercent);
    Serial.println("%");
  }

  Serial.println("Derek assignments:");
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    Serial.print("  Derek ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(canTypeName(gDerekCanTypes[i]));
  }

  Serial.print("Cluster ID: ");
  Serial.println(gClusterId);
  Serial.print("Software version: ");
  Serial.print(kSoftwareVersionMajor);
  Serial.print(".");
  Serial.print(kSoftwareVersionMinor);
  Serial.print(".");
  Serial.println(kSoftwareVersionRevision);
  Serial.print("Self-test mode: ");
  Serial.println(gSelfTestMode == kSelfTestModeFull ? "full" : (gSelfTestMode == kSelfTestModeSingle ? "single" : "off"));
}

void printSerialHelp() {
  if (!kEnableSerialLogs) {
    return;
  }

  Serial.println("Commands:");
  Serial.println("  help");
  Serial.println("  config");
  Serial.println("  type <derek 1-8> <smallest|small|medium|tall|tallest>");
  Serial.println("  profile <type> <minPercent 0-100> <maxPercent 0-100>");
  Serial.println("  defaults");
  Serial.println("  selftest off|single|full");
}

void handleSerialCommand(char* commandLine) {
  char* command = strtok(commandLine, " \t");
  if (command == nullptr) {
    return;
  }

  if (strcasecmp(command, "help") == 0) {
    printSerialHelp();
    return;
  }

  if (strcasecmp(command, "config") == 0) {
    printSerialConfig();
    return;
  }

  if (strcasecmp(command, "selftest") == 0) {
    const char* modeText = strtok(nullptr, " \t");
    if (modeText == nullptr) {
      Serial.println("Usage: selftest off|single|full");
      return;
    }

    if (strcasecmp(modeText, "off") == 0) {
      gSelfTestMode = kSelfTestModeOff;
      applyFailsafeTargets();
      Serial.println("Self-test off. MCU control enabled.");
      return;
    }
    if (strcasecmp(modeText, "single") == 0) {
      gSelfTestMode = kSelfTestModeSingle;
      gLastSelfTestAtMs = 0;
      Serial.println("Single-Derek self-test enabled.");
      return;
    }
    if (strcasecmp(modeText, "full") == 0) {
      gSelfTestMode = kSelfTestModeFull;
      gLastFullClusterTestAtMs = 0;
      Serial.println("Full-cluster self-test enabled.");
      return;
    }

    Serial.println("Usage: selftest off|single|full");
    return;
  }

  if (strcasecmp(command, "defaults") == 0) {
    loadDefaultDerekTypeConfig();
    saveDerekTypeConfig();
    Serial.println("Default Derek type config saved.");
    printSerialConfig();
    return;
  }

  if (strcasecmp(command, "type") == 0) {
    const char* derekText = strtok(nullptr, " \t");
    const char* typeText = strtok(nullptr, " \t");
    if (derekText == nullptr || typeText == nullptr) {
      Serial.println("Usage: type <derek 1-8> <smallest|small|medium|tall|tallest>");
      return;
    }

    const int derekNumber = atoi(derekText);
    CanType canType = kCanMedium;
    if (derekNumber < 1 || derekNumber > kMaxDerricks || !parseCanType(typeText, canType)) {
      Serial.println("Invalid Derek number or can type.");
      return;
    }

    gDerekCanTypes[derekNumber - 1] = canType;
    saveDerekTypeConfig();
    Serial.println("Derek type saved.");
    printSerialConfig();
    return;
  }

  if (strcasecmp(command, "profile") == 0) {
    const char* typeText = strtok(nullptr, " \t");
    const char* minText = strtok(nullptr, " \t");
    const char* maxText = strtok(nullptr, " \t");
    CanType canType = kCanMedium;
    uint8_t minPercent = 0;
    uint8_t maxPercent = 0;
    if (typeText == nullptr || minText == nullptr || maxText == nullptr || !parseCanType(typeText, canType) ||
        !parsePercent(minText, minPercent) || !parsePercent(maxText, maxPercent) || minPercent > maxPercent) {
      Serial.println("Usage: profile <type> <minPercent 0-100> <maxPercent 0-100>");
      return;
    }

    gCanLiftProfiles[canType] = {minPercent, maxPercent};
    saveDerekTypeConfig();
    Serial.println("Can profile saved.");
    printSerialConfig();
    return;
  }

  Serial.println("Unknown command. Type help.");
}

void updateSerialCommands() {
  if (!kEnableSerialLogs) {
    return;
  }

  while (Serial.available() > 0) {
    const char next = static_cast<char>(Serial.read());
    if (next == '\r') {
      continue;
    }
    if (next == '\n') {
      gSerialCommandBuffer[gSerialCommandLength] = '\0';
      handleSerialCommand(gSerialCommandBuffer);
      gSerialCommandLength = 0;
      gSerialCommandBuffer[0] = '\0';
      continue;
    }
    if (gSerialCommandLength < kSerialCommandBufferSize - 1) {
      gSerialCommandBuffer[gSerialCommandLength++] = next;
    } else {
      gSerialCommandLength = 0;
      gSerialCommandBuffer[0] = '\0';
      Serial.println("Command too long.");
    }
  }
}

bool isProtocolPacketValid(const uint8_t* data, size_t len, uint8_t expectedKind) {
  if (len < sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t)) {
    return false;
  }

  const uint16_t magic = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  if (magic != kProtocolMagic) {
    return false;
  }

  if (data[2] != kProtocolVersion) {
    return false;
  }

  return data[3] == expectedKind;
}

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

void ensureControllerPeer(const uint8_t* macAddress) {
  const bool wasKnown = gControllerMacKnown;
  memcpy(gControllerMac, macAddress, sizeof(gControllerMac));
  gControllerMacKnown = addEspNowPeer(macAddress);
  if (!wasKnown && gControllerMacKnown) {
    gOledRuntimeRefreshPending = true;
  }
}

uint16_t buildHealthFlags(uint32_t nowMs) {
  uint16_t flags = 0;
  if (gServosReady) {
    flags |= kHealthServosReady;
  }
  if (gLedsReady) {
    flags |= kHealthLedsReady;
  }
  if (gAudio1Ready) {
    flags |= kHealthAudio1Ready;
  }
  if (gAudio2Ready) {
    flags |= kHealthAudio2Ready;
  }
  if (gPirReady) {
    flags |= kHealthPirReady;
  }
  if (gControllerMacKnown) {
    flags |= kHealthControllerLinked;
  }
  if (gLastCommandAtMs == 0 || (nowMs - gLastCommandAtMs) > kCommandTimeoutMs) {
    flags |= kHealthCommandStale;
  }
  if (gServoPwmEnabled) {
    flags |= kHealthServoPwmEnabled;
  }
  return flags;
}

void applyFailsafeTargets() {
  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    gDerricks[i].targetLift = gDerricks[i].calibration.liftMin;
    gDerricks[i].targetPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].output.spotlightColor = {0, 0, 0};
    gDerricks[i].output.rightEyeColor = {0, 0, 0};
    gDerricks[i].output.leftEyeColor = {0, 0, 0};
    markServoOutputDirty(i);
  }

  writeLedOutputs();
}

uint8_t applyPanCalibration(uint8_t derrickIndex, uint8_t requestedPan) {
  const DerekCalibration& calibration = gDerricks[derrickIndex].calibration;
  return clampU8(requestedPan, calibration.panMin, calibration.panMax);
}

uint8_t percentToLiftAngle(uint8_t percent) {
  const uint8_t clampedPercent = clampU8(percent, 0, 100);
  return map(clampedPercent, 0, 100, kLiftTravelMinAngle, kLiftTravelMaxAngle);
}

uint8_t applyLiftCalibration(uint8_t derrickIndex, uint8_t requestedLift) {
  const DerekCalibration& calibration = gDerricks[derrickIndex].calibration;
  const CanLiftProfile& canProfile = gCanLiftProfiles[gDerekCanTypes[derrickIndex]];
  const uint8_t canLiftMin = percentToLiftAngle(canProfile.minPercent);
  const uint8_t canLiftMax = percentToLiftAngle(canProfile.maxPercent);
  const uint8_t effectiveLiftMin = max<uint8_t>(calibration.liftMin, canLiftMin);
  const uint8_t effectiveLiftMax = min<uint8_t>(calibration.liftMax, canLiftMax);
  return clampU8(requestedLift, effectiveLiftMin, effectiveLiftMax);
}

bool writeI2cBytes(uint8_t address, const uint8_t* data, size_t len) {
  Wire.beginTransmission(address);
  Wire.write(data, len);
  return Wire.endTransmission() == 0;
}

bool initializePca9685() {
  pinMode(kPcaOePin, OUTPUT);
  digitalWrite(kPcaOePin, HIGH);
  gServoPwmEnabled = false;

  const bool ok = gServoDriver.begin();
  if (ok) {
    gServoDriver.setPWMFreq(kServoPwmFrequencyHz);
    delay(10);
  }
  digitalWrite(kPcaOePin, HIGH);
  return ok;
}

uint16_t angleToServoPulseUs(uint8_t angle) {
  return map(angle, 0, 180, kServoMinPulseUs, kServoMaxPulseUs);
}

uint16_t pulseUsToPwmTicks(uint16_t pulseUs) {
  const uint32_t numerator = static_cast<uint32_t>(pulseUs) * kServoPwmFrequencyHz * 4096UL;
  return static_cast<uint16_t>(constrain((numerator + 500000UL) / 1000000UL, 0UL, 4095UL));
}

void setServoPwmEnabled(bool enabled) {
  if (!gServosReady || gServoPwmEnabled == enabled) {
    return;
  }

  digitalWrite(kPcaOePin, enabled ? LOW : HIGH);
  gServoPwmEnabled = enabled;
}

bool areActiveServosAtTargets() {
  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    const DerekState& derrick = gDerricks[i];
    if (derrick.currentPan != derrick.targetPan || derrick.currentLift != derrick.targetLift) {
      return false;
    }
  }
  return true;
}

bool areServoOutputsPending() {
  return gServoDirtyMask != 0;
}

void markServoOutputDirty(uint8_t derrickIndex) {
  if (derrickIndex < kMaxDerricks) {
    gServoDirtyMask |= static_cast<uint8_t>(1U << derrickIndex);
    gServoRefreshPassesRemaining[derrickIndex] = kServoOutputRefreshPasses;
  }
}

void markActiveServoOutputsDirty() {
  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    markServoOutputDirty(i);
  }
}

MovementSpeed normalizeMovementSpeed(uint8_t movementSpeed) {
  switch (movementSpeed) {
    case kMovementSpeedMedium:
      return kMovementSpeedMedium;
    case kMovementSpeedSlow:
      return kMovementSpeedSlow;
    case kMovementSpeedFast:
    default:
      return kMovementSpeedFast;
  }
}

bool writeServoChannel(uint8_t channel, uint8_t angle) {
  const uint8_t result = gServoDriver.setPWM(channel, 0, pulseUsToPwmTicks(angleToServoPulseUs(angle)));
  ++gServoI2cWrites;
  if (result != 0) {
    ++gServoI2cFailures;
    return false;
  }
  return true;
}

bool writeServoOutputs(uint8_t derrickIndex) {
  if (derrickIndex >= kMaxDerricks || !gServosReady) {
    return false;
  }

  bool ok = true;
  const int8_t panChannel = kServoPanChannels[derrickIndex];
  const int8_t liftChannel = kServoLiftChannels[derrickIndex];
  if (panChannel >= 0) {
    ok = writeServoChannel(static_cast<uint8_t>(panChannel), gDerricks[derrickIndex].currentPan) && ok;
  }
  if (liftChannel >= 0) {
    ok = writeServoChannel(static_cast<uint8_t>(liftChannel), gDerricks[derrickIndex].currentLift) && ok;
  }
  return ok;
}

void serviceServoOutputs(uint32_t nowMs) {
  if (!areServoOutputsPending()) {
    return;
  }
  if ((nowMs - gLastServoOutputServiceAtMs) < kServoOutputServiceIntervalMs) {
    return;
  }
  gLastServoOutputServiceAtMs = nowMs;

  setServoPwmEnabled(true);
  gLastServoMotionAtMs = nowMs;

  uint8_t serviced = 0;
  uint8_t checked = 0;
  while (serviced < kServoDereksPerService && checked < kMaxDerricks) {
    const uint8_t derrickIndex = gNextServoServiceIndex;
    gNextServoServiceIndex = (gNextServoServiceIndex + 1) % kMaxDerricks;
    ++checked;

    const uint8_t mask = static_cast<uint8_t>(1U << derrickIndex);
    if ((gServoDirtyMask & mask) == 0) {
      continue;
    }

    const bool writeOk = writeServoOutputs(derrickIndex);
    if (writeOk && gServoRefreshPassesRemaining[derrickIndex] > 0) {
      --gServoRefreshPassesRemaining[derrickIndex];
    }
    if (gServoRefreshPassesRemaining[derrickIndex] == 0) {
      gServoDirtyMask &= ~mask;
    }
    ++serviced;
  }
}

void writeLedOutputs() {
  if (!gLedsReady) {
    return;
  }

  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    const uint16_t base = i * kLedsPerDerek;
    const DerekCommand& output = gDerricks[i].output;
    for (uint8_t spotlight = 0; spotlight < kSpotlightLedsPerDerek; ++spotlight) {
      gLedStrip.setPixelColor(
          base + spotlight,
          gLedStrip.Color(output.spotlightColor.r, output.spotlightColor.g, output.spotlightColor.b));
    }
    gLedStrip.setPixelColor(
        base + kSpotlightLedsPerDerek,
        gLedStrip.Color(output.rightEyeColor.r, output.rightEyeColor.g, output.rightEyeColor.b));
    gLedStrip.setPixelColor(
        base + kSpotlightLedsPerDerek + kRightEyeLedsPerDerek,
        gLedStrip.Color(output.leftEyeColor.r, output.leftEyeColor.g, output.leftEyeColor.b));
  }

  gLedStrip.show();
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
      0xA1, 0xC8, 0xDA, 0x12, 0x81, 0x7F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
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
    case '%':
      return column == 0 ? 0x63 : (column == 1 ? 0x13 : (column == 2 ? 0x08 : (column == 3 ? 0x64 : 0x63)));
    case ' ':
    default:
      return 0x00;
  }
}

void writeOledLine(uint8_t page, const char* text) {
  if (!gOledReady || page >= 8) {
    return;
  }

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
  if (!gOledReady) {
    return;
  }
  for (uint8_t page = 0; page < 8; ++page) {
    writeOledLine(page, "");
  }
}

const char* selfTestModeName() {
  switch (gSelfTestMode) {
    case kSelfTestModeSingle:
      return "SINGLE";
    case kSelfTestModeFull:
      return "FULL";
    default:
      return "OFF";
  }
}

void showOledStartupStatus(const char* step) {
  if (!gOledReady) {
    return;
  }

  char line[22] = {};
  clearOled();
  snprintf(
      line,
      sizeof(line),
      "CL%u V%u.%u.%u",
      gClusterId,
      kSoftwareVersionMajor,
      kSoftwareVersionMinor,
      kSoftwareVersionRevision);
  writeOledLine(0, line);
  writeOledLine(1, "STARTUP");
  writeOledLine(2, step);
}

void showOledRuntimeSummary(const char* status) {
  if (!gOledReady) {
    return;
  }

  char line[22] = {};
  clearOled();

  snprintf(
      line,
      sizeof(line),
      "CL%u V%u.%u.%u",
      gClusterId,
      kSoftwareVersionMajor,
      kSoftwareVersionMinor,
      kSoftwareVersionRevision);
  writeOledLine(0, line);

  writeOledLine(1, status);
  snprintf(line, sizeof(line), "MCU %s", gControllerMacKnown ? "LINKED" : "WAITING");
  writeOledLine(2, line);

  snprintf(line, sizeof(line), "ID %u ACTIVE %u", gClusterId, gActiveDerricks);
  writeOledLine(3, line);

  snprintf(line, sizeof(line), "S%u L%u A%u%u P%u", gServosReady, gLedsReady, gAudio1Ready, gAudio2Ready, gPirReady);
  writeOledLine(4, line);

  snprintf(line, sizeof(line), "PIR %u PWM %s", gPirStateBits, gServoPwmEnabled ? "ON" : "REST");
  writeOledLine(5, line);

  snprintf(line, sizeof(line), "TEST %s", selfTestModeName());
  writeOledLine(6, line);

  snprintf(line, sizeof(line), "SEQ %u", gLastSequence);
  writeOledLine(7, line);
}

void serviceOledRuntimeSummary() {
  if (!gOledRuntimeRefreshPending || !areActiveServosAtTargets() || gServoPwmEnabled) {
    return;
  }

  gOledRuntimeRefreshPending = false;
  showOledRuntimeSummary(gControllerMacKnown ? "MCU LINKED" : "WAIT MCU");
}

void writeOledTestOutput(uint8_t derrickIndex, uint8_t step) {
  if (gOledReady) {
    uint8_t pageData[128];
    for (uint8_t page = 0; page < 8; ++page) {
      sendOledCommand(0xB0 + page);
      sendOledCommand(0x00);
      sendOledCommand(0x10);
      for (uint8_t col = 0; col < sizeof(pageData); ++col) {
        const bool inDerekBand = col >= derrickIndex * 16 && col < (derrickIndex + 1) * 16;
        pageData[col] = inDerekBand ? 0xFF : ((col + page + step) % 16 == 0 ? 0x18 : 0x00);
      }
      sendOledData(pageData, sizeof(pageData));
    }
  }

  if (kEnableSerialLogs) {
    Serial.print("OLED TEST: Derek ");
    Serial.print(derrickIndex + 1);
    Serial.print(" step ");
    Serial.println(step);
  }
}

void writeOledSequenceStage(uint8_t stage) {
  if (gOledReady) {
    uint8_t pageData[128];
    const uint8_t activeWidth = static_cast<uint8_t>((stage + 1) * 16);

    for (uint8_t page = 0; page < 8; ++page) {
      sendOledCommand(0xB0 + page);
      sendOledCommand(0x00);
      sendOledCommand(0x10);

      for (uint8_t col = 0; col < sizeof(pageData); ++col) {
        if (page <= 1) {
          pageData[col] = col < activeWidth ? 0xFF : 0x00;
        } else if (page == 3 || page == 4) {
          pageData[col] = (col / 16) == stage ? 0xFF : 0x18;
        } else if (page == 6) {
          pageData[col] = (col + stage) % 8 == 0 ? 0xFF : 0x00;
        } else {
          pageData[col] = 0x00;
        }
      }

      sendOledData(pageData, sizeof(pageData));
    }
  }

  if (kEnableSerialLogs) {
    static const char* const kStageNames[] = {
        "white leds",
        "raise lifts",
        "eyes left",
        "eyes red",
        "eyes right",
        "eyes green",
        "center lower",
        "leds off",
    };

    Serial.print("OLED SEQUENCE: ");
    Serial.println(kStageNames[stage]);
  }
}

void updateAudioOutputs() {
  if (gPendingAudioA) {
    gPendingAudioA = false;
    if (gAudio1Ready) {
      gDfPlayer1.play(gPendingAudioTrackA);
    }
  }

  if (gPendingAudioB) {
    gPendingAudioB = false;
    if (gAudio2Ready) {
      gDfPlayer2.play(gPendingAudioTrackB);
    }
  }
}

void runClusterSelfTest(uint32_t nowMs) {
  if (gSelfTestMode != kSelfTestModeSingle || (nowMs - gLastSelfTestAtMs) < kSelfTestIntervalMs) {
    return;
  }

  gLastSelfTestAtMs = nowMs;
  gActiveDerricks = kMaxDerricks;

  const uint8_t derrickIndex = gSelfTestStep % kMaxDerricks;
  const uint8_t motionPhase = (gSelfTestStep / kMaxDerricks) % 3;
  const uint8_t testPanTargets[3] = {10, 90, 170};
  const uint8_t testLiftTargets[3] = {0, 170, 85};
  const RgbColor testEyeColors[3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}};
  const RgbColor testCanColors[3] = {{40, 0, 0}, {0, 40, 0}, {0, 0, 40}};
  const RgbColor testExteriorColors[3] = {{255, 80, 0}, {80, 0, 255}, {255, 255, 255}};

  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    DerekState& derrick = gDerricks[i];
    if (i == derrickIndex) {
      derrick.targetPan = applyPanCalibration(i, testPanTargets[motionPhase]);
      derrick.targetLift = applyLiftCalibration(i, testLiftTargets[motionPhase]);
      derrick.output = {
          derrick.targetPan,
          derrick.targetLift,
          testEyeColors[motionPhase],
          testCanColors[motionPhase],
          testExteriorColors[motionPhase],
      };
    } else {
      derrick.targetPan = derrick.calibration.panCenter;
      derrick.targetLift = derrick.calibration.liftMin;
      derrick.output = {derrick.targetPan, derrick.targetLift, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    }
    markServoOutputDirty(i);
  }

  if (derrickIndex == 0 && motionPhase == 0) {
    gPendingAudioTrackA = kSelfTestAudioTrack;
    gPendingAudioTrackB = kSelfTestAudioTrack;
    gPendingAudioA = true;
    gPendingAudioB = true;
  }

  writeLedOutputs();
  ++gSelfTestStep;
}

void setAllDerekOutputs(
    uint8_t pan,
    uint8_t lift,
    RgbColor spotlightColor,
    RgbColor rightEyeColor,
    RgbColor leftEyeColor) {
  gActiveDerricks = kMaxDerricks;

  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    DerekState& derrick = gDerricks[i];
    derrick.targetPan = applyPanCalibration(i, pan);
    derrick.targetLift = applyLiftCalibration(i, lift);
    derrick.output = {derrick.targetPan, derrick.targetLift, spotlightColor, rightEyeColor, leftEyeColor};
    markServoOutputDirty(i);
  }

  writeLedOutputs();
}

void runFullClusterSequenceTest(uint32_t nowMs) {
  if (gSelfTestMode != kSelfTestModeFull || (nowMs - gLastFullClusterTestAtMs) < kFullClusterTestStageMs) {
    return;
  }

  gLastFullClusterTestAtMs = nowMs;

  constexpr RgbColor kOff = {0, 0, 0};
  constexpr RgbColor kWhite = {255, 255, 255};
  constexpr RgbColor kRed = {255, 0, 0};
  constexpr RgbColor kGreen = {0, 255, 0};

  switch (gFullClusterTestStage) {
    case 0:
      setAllDerekOutputs(90, 0, kWhite, kWhite, kWhite);
      break;
    case 1:
      setAllDerekOutputs(90, 170, kWhite, kWhite, kWhite);
      break;
    case 2:
      setAllDerekOutputs(10, 170, kWhite, kWhite, kWhite);
      break;
    case 3:
      setAllDerekOutputs(10, 170, kRed, kWhite, kWhite);
      break;
    case 4:
      setAllDerekOutputs(170, 170, kRed, kWhite, kWhite);
      break;
    case 5:
      setAllDerekOutputs(170, 170, kGreen, kWhite, kWhite);
      break;
    case 6:
      setAllDerekOutputs(90, 0, kGreen, kWhite, kWhite);
      break;
    default:
      setAllDerekOutputs(90, 0, kOff, kOff, kOff);
      break;
  }

  gFullClusterTestStage = (gFullClusterTestStage + 1) % 8;
}

void updateMotion(uint32_t nowMs) {
  if ((nowMs - gLastMotionUpdateAtMs) < kMotionUpdateIntervalMs) {
    return;
  }
  gLastMotionUpdateAtMs = nowMs;

  bool anyServoMoved = false;
  const uint8_t panStep = gMovementSpeed == kMovementSpeedMedium ? kMediumPanStepPerTick : kPanStepPerTick;
  const uint8_t liftStep = gMovementSpeed == kMovementSpeedMedium ? kMediumLiftStepPerTick : kLiftStepPerTick;

  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    DerekState& derrick = gDerricks[i];
    const uint8_t previousPan = derrick.currentPan;
    const uint8_t previousLift = derrick.currentLift;

    if (gMovementSpeed == kMovementSpeedFast) {
      derrick.currentPan = derrick.targetPan;
      derrick.currentLift = derrick.targetLift;
    } else {
      if (derrick.currentPan < derrick.targetPan) {
        derrick.currentPan = min<uint8_t>(derrick.targetPan, derrick.currentPan + panStep);
      } else if (derrick.currentPan > derrick.targetPan) {
        const uint8_t nextPan = derrick.currentPan > panStep ? derrick.currentPan - panStep : 0;
        derrick.currentPan = max<uint8_t>(derrick.targetPan, nextPan);
      }

      if (derrick.currentLift < derrick.targetLift) {
        derrick.currentLift = min<uint8_t>(derrick.targetLift, derrick.currentLift + liftStep);
      } else if (derrick.currentLift > derrick.targetLift) {
        const uint8_t nextLift = derrick.currentLift > liftStep ? derrick.currentLift - liftStep : 0;
        derrick.currentLift = max<uint8_t>(derrick.targetLift, nextLift);
      }
    }

    if (derrick.currentPan != previousPan || derrick.currentLift != previousLift) {
      anyServoMoved = true;
      markServoOutputDirty(i);
    }
  }

  if (anyServoMoved || !areActiveServosAtTargets() || areServoOutputsPending()) {
    setServoPwmEnabled(true);
    gLastServoMotionAtMs = nowMs;
    return;
  }

  if (gServoPwmEnabled && !areServoOutputsPending() && (nowMs - gLastServoMotionAtMs) >= kServoSoftRestDelayMs) {
    setServoPwmEnabled(false);
  }
}

void updatePirInputs() {
  uint8_t pirBits = 0;
  bool anyAssigned = false;

  for (uint8_t i = 0; i < 3; ++i) {
    if (kPirPins[i] < 0) {
      continue;
    }

    anyAssigned = true;
    pirBits |= (digitalRead(kPirPins[i]) ? 1 : 0) << i;
  }

  gPirReady = anyAssigned;
  gPirStateBits = pirBits;
}

void sendStatusPacket(uint8_t kind, uint32_t nowMs) {
  ClusterStatusPacket status = {};
  status.magic = kProtocolMagic;
  status.version = kProtocolVersion;
  status.kind = kind;
  status.clusterId = gClusterId;
  status.sequence = gLastSequence;
  status.activeDerricks = gActiveDerricks;
  status.pirStateBits = gPirStateBits;
  status.healthFlags = buildHealthFlags(nowMs);
  status.uptimeMs = nowMs;
  status.lastCommandAgeMs = gLastCommandAtMs == 0 ? UINT32_MAX : nowMs - gLastCommandAtMs;

  const uint8_t* destination = gControllerMacKnown ? gControllerMac : kBroadcastMac;
  esp_now_send(destination, reinterpret_cast<const uint8_t*>(&status), sizeof(status));
}

void handleCommand(const ClusterCommandPacket& packet, uint32_t nowMs) {
  if (packet.clusterId != kBroadcastClusterId && packet.clusterId != gClusterId) {
    return;
  }

  gLastSequence = packet.sequence;
  gLastCommandAtMs = nowMs;
  gActiveDerricks = packet.activeDerricks == 0 ? kMaxDerricks : min<uint8_t>(packet.activeDerricks, kMaxDerricks);
  gMovementSpeed = normalizeMovementSpeed(packet.movementSpeed);

  if ((packet.flags & kCommandFlagDiscovery) == 0) {
    gSelfTestMode = kSelfTestModeOff;
  }

  if (packet.flags & kCommandFlagEmergencyHide) {
    applyFailsafeTargets();
  }

  if (packet.flags & kCommandFlagApplyOutputs) {
    for (uint8_t i = 0; i < gActiveDerricks; ++i) {
      gDerricks[i].targetPan = applyPanCalibration(i, packet.derricks[i].pan);
      gDerricks[i].targetLift = applyLiftCalibration(i, packet.derricks[i].lift);
      gDerricks[i].output = packet.derricks[i];
      markServoOutputDirty(i);
    }

    writeLedOutputs();
  }

  if (packet.flags & kCommandFlagAudioA) {
    gPendingAudioTrackA = packet.audioTrackA;
    gPendingAudioA = true;
  }

  if (packet.flags & kCommandFlagAudioB) {
    gPendingAudioTrackB = packet.audioTrackB;
    gPendingAudioB = true;
  }

  if (packet.flags & kCommandFlagRequestStatus) {
    sendStatusPacket(kMessageStatus, nowMs);
  }
}

void processPendingCommand(uint32_t nowMs) {
  for (uint8_t processed = 0; processed < kMaxQueuedCommandsPerLoop; ++processed) {
    ClusterCommandPacket packet = {};
    uint8_t senderMac[6] = {};
    bool hasCommand = false;

    portENTER_CRITICAL(&gPendingCommandMux);
    if (gCommandQueueCount > 0) {
      const QueuedCommand& queued = gCommandQueue[gCommandQueueHead];
      memcpy(&packet, &queued.packet, sizeof(packet));
      memcpy(senderMac, queued.senderMac, sizeof(senderMac));
      gCommandQueueHead = (gCommandQueueHead + 1) % kCommandQueueDepth;
      --gCommandQueueCount;
      hasCommand = true;
    }
    portEXIT_CRITICAL(&gPendingCommandMux);

    if (!hasCommand) {
      return;
    }

    ensureControllerPeer(senderMac);
    handleCommand(packet, nowMs);
  }
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onEspNowReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (info == nullptr || data == nullptr || len <= 0) {
    return;
  }

  const uint8_t* senderMac = info->src_addr;
#else
void onEspNowReceive(const uint8_t* senderMac, const uint8_t* data, int len) {
  if (senderMac == nullptr || data == nullptr || len <= 0) {
    return;
  }
#endif
  if (!isProtocolPacketValid(data, static_cast<size_t>(len), kMessageCommand)) {
    return;
  }

  if (static_cast<size_t>(len) != sizeof(ClusterCommandPacket)) {
    return;
  }

  portENTER_CRITICAL(&gPendingCommandMux);
  if (gCommandQueueCount >= kCommandQueueDepth) {
    gCommandQueueHead = (gCommandQueueHead + 1) % kCommandQueueDepth;
    --gCommandQueueCount;
    ++gCommandQueueOverflows;
  }

  QueuedCommand& queued = gCommandQueue[gCommandQueueTail];
  memcpy(&queued.packet, data, sizeof(queued.packet));
  memcpy(queued.senderMac, senderMac, sizeof(queued.senderMac));
  gCommandQueueTail = (gCommandQueueTail + 1) % kCommandQueueDepth;
  ++gCommandQueueCount;
  portEXIT_CRITICAL(&gPendingCommandMux);
}

#if ESP_ARDUINO_VERSION_MAJOR >= 3
void onEspNowSend(const wifi_tx_info_t* txInfo, esp_now_send_status_t status) {
  (void)txInfo;
#else
void onEspNowSend(const uint8_t* macAddr, esp_now_send_status_t status) {
  (void)macAddr;
#endif
  (void)status;
}

bool initializeEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false, true);

  if (esp_now_init() != ESP_OK) {
    return false;
  }

  addEspNowPeer(kBroadcastMac);
  esp_now_register_recv_cb(onEspNowReceive);
  esp_now_register_send_cb(onEspNowSend);
  return true;
}

void initializeDerrickState() {
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    gDerricks[i].calibration = {10, 90, 170, kLiftTravelMinAngle, kLiftTravelMaxAngle};
    gDerricks[i].currentPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].targetPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].currentLift = applyLiftCalibration(i, kLiftTravelMinAngle);
    gDerricks[i].targetLift = gDerricks[i].currentLift;
    gDerricks[i].output = {gDerricks[i].currentPan, gDerricks[i].currentLift, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  }
}

void initializePirInputs() {
  bool anyAssigned = false;
  for (uint8_t i = 0; i < 3; ++i) {
    if (kPirPins[i] >= 0) {
      pinMode(kPirPins[i], INPUT);
      anyAssigned = true;
    }
  }
  gPirReady = anyAssigned;
}

void initializeHardwareAvailability() {
  Wire.begin(kI2cSdaPin, kI2cSclPin);
  Wire.setClock(kI2cClockHz);
  gServosReady = initializePca9685();
  gOledReady = initializeOled();
  showOledStartupStatus("OLED READY");

  gLedStrip.begin();
  gLedStrip.clear();
  gLedStrip.show();
  gLedsReady = true;
  showOledStartupStatus(gServosReady ? "SERVO OK" : "SERVO FAIL");

  gDfPlayer1Serial.begin(9600, SERIAL_8N1, kDfPlayer1RxPin, kDfPlayer1TxPin);
  gDfPlayer2Serial.begin(9600, SERIAL_8N1, kDfPlayer2RxPin, kDfPlayer2TxPin);
  gAudio1Ready = gDfPlayer1.begin(gDfPlayer1Serial);
  gAudio2Ready = gDfPlayer2.begin(gDfPlayer2Serial);
  showOledStartupStatus("AUDIO CHECKED");

  if (gAudio1Ready) {
    gDfPlayer1.volume(22);
  }
  if (gAudio2Ready) {
    gDfPlayer2.volume(22);
  }
}

void publishIfNeeded(uint32_t nowMs) {
  const bool pirChanged = gPirStateBits != gLastReportedPirBits;
  if ((nowMs - gLastStatusAtMs) >= kStatusIntervalMs || pirChanged) {
    sendStatusPacket(kMessageStatus, nowMs);
    gLastStatusAtMs = nowMs;
    gLastReportedPirBits = gPirStateBits;
  }

  if (!gControllerMacKnown && (nowMs - gLastRegistrationAtMs) >= kRegistrationIntervalMs) {
    sendStatusPacket(kMessageRegister, nowMs);
    gLastRegistrationAtMs = nowMs;
  }
}

}  // namespace

void setup() {
  if (kEnableSerialLogs) {
    Serial.begin(115200);
  }

  initializeClusterIdInputs();
  loadDerekTypeConfig();
  initializeDerrickState();
  initializePirInputs();
  initializeHardwareAvailability();
  showOledStartupStatus("HARDWARE READY");

  if (!initializeEspNow()) {
    logLine("ESP-NOW init failed.");
    showOledStartupStatus("ESPNOW FAIL");
    return;
  }
  showOledStartupStatus("ESPNOW READY");

  logLine("Cluster controller ready.");
  printSerialHelp();
  printSerialConfig();
  showOledRuntimeSummary("WAIT MCU");
}

void loop() {
  const uint32_t nowMs = millis();

  updateSerialCommands();
  updatePirInputs();
  processPendingCommand(nowMs);

  if (gLastCommandAtMs != 0 && (nowMs - gLastCommandAtMs) > kCommandTimeoutMs) {
    applyFailsafeTargets();
  }

  runClusterSelfTest(nowMs);
  runFullClusterSequenceTest(nowMs);
  updateMotion(nowMs);
  serviceServoOutputs(nowMs);
  updateAudioOutputs();
  publishIfNeeded(nowMs);
  serviceOledRuntimeSummary();
}
