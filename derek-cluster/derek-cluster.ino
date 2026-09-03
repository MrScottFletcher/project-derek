#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#ifndef ARDUINO_ARCH_ESP32
#error "This sketch targets ESP32-class boards."
#endif

namespace {

constexpr bool kEnableSerialLogs = true;
constexpr uint8_t kProtocolVersion = 1;
constexpr uint16_t kProtocolMagic = 0xD311;
constexpr uint8_t kClusterId = 1;
constexpr uint8_t kMaxDerricks = 8;
constexpr uint32_t kStatusIntervalMs = 250;
constexpr uint32_t kRegistrationIntervalMs = 1000;
constexpr uint32_t kCommandTimeoutMs = 1500;
constexpr uint32_t kMotionUpdateIntervalMs = 20;
constexpr uint8_t kPanStepPerTick = 2;
constexpr uint8_t kLiftStepPerTick = 2;
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Hardware mappings are intentionally left unassigned until the real wiring is known.
constexpr int8_t kPirPins[3] = {-1, -1, -1};
constexpr int8_t kServoPanChannels[kMaxDerricks] = {-1, -1, -1, -1, -1, -1, -1, -1};
constexpr int8_t kServoLiftChannels[kMaxDerricks] = {-1, -1, -1, -1, -1, -1, -1, -1};
constexpr int8_t kDfPlayer1RxPin = -1;
constexpr int8_t kDfPlayer1TxPin = -1;
constexpr int8_t kDfPlayer2RxPin = -1;
constexpr int8_t kDfPlayer2TxPin = -1;
constexpr int8_t kLedDataPin = -1;

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

DerekState gDerricks[kMaxDerricks];
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
uint16_t gLastSequence = 0;
uint32_t gLastCommandAtMs = 0;
uint32_t gLastStatusAtMs = 0;
uint32_t gLastRegistrationAtMs = 0;
uint32_t gLastMotionUpdateAtMs = 0;
uint16_t gPendingAudioTrackA = 0;
uint16_t gPendingAudioTrackB = 0;
bool gPendingAudioA = false;
bool gPendingAudioB = false;

void writeLedOutputs();

void logLine(const char* message) {
  if (kEnableSerialLogs) {
    Serial.println(message);
  }
}

uint8_t clampU8(uint8_t value, uint8_t minimum, uint8_t maximum) {
  return value < minimum ? minimum : (value > maximum ? maximum : value);
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
  memcpy(gControllerMac, macAddress, sizeof(gControllerMac));
  gControllerMacKnown = addEspNowPeer(macAddress);
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
  return flags;
}

void applyFailsafeTargets() {
  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    gDerricks[i].targetLift = gDerricks[i].calibration.liftMin;
    gDerricks[i].targetPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].output.eyeColor = {0, 0, 0};
    gDerricks[i].output.canColor = {0, 0, 0};
    gDerricks[i].output.exteriorColor = {0, 0, 0};
  }

  writeLedOutputs();
}

uint8_t applyPanCalibration(uint8_t derrickIndex, uint8_t requestedPan) {
  const DerekCalibration& calibration = gDerricks[derrickIndex].calibration;
  return clampU8(requestedPan, calibration.panMin, calibration.panMax);
}

uint8_t applyLiftCalibration(uint8_t derrickIndex, uint8_t requestedLift) {
  const DerekCalibration& calibration = gDerricks[derrickIndex].calibration;
  return clampU8(requestedLift, calibration.liftMin, calibration.liftMax);
}

void writeServoOutputs(uint8_t derrickIndex) {
  (void)derrickIndex;
  // TODO: Drive the actual PWM servo controller here once channel mappings are known.
}

void writeLedOutputs() {
  // TODO: Push the current LED state to the addressable strip once LED ordering is defined.
}

void updateAudioOutputs() {
  if (gPendingAudioA) {
    gPendingAudioA = false;
    // TODO: Trigger DFPlayer A using the assigned serial interface.
  }

  if (gPendingAudioB) {
    gPendingAudioB = false;
    // TODO: Trigger DFPlayer B using the assigned serial interface.
  }
}

void updateMotion(uint32_t nowMs) {
  if ((nowMs - gLastMotionUpdateAtMs) < kMotionUpdateIntervalMs) {
    return;
  }
  gLastMotionUpdateAtMs = nowMs;

  for (uint8_t i = 0; i < gActiveDerricks; ++i) {
    DerekState& derrick = gDerricks[i];

    if (derrick.currentPan < derrick.targetPan) {
      derrick.currentPan = min<uint8_t>(derrick.targetPan, derrick.currentPan + kPanStepPerTick);
    } else if (derrick.currentPan > derrick.targetPan) {
      const uint8_t nextPan =
          derrick.currentPan > kPanStepPerTick ? derrick.currentPan - kPanStepPerTick : 0;
      derrick.currentPan = max<uint8_t>(derrick.targetPan, nextPan);
    }

    if (derrick.currentLift < derrick.targetLift) {
      derrick.currentLift = min<uint8_t>(derrick.targetLift, derrick.currentLift + kLiftStepPerTick);
    } else if (derrick.currentLift > derrick.targetLift) {
      const uint8_t nextLift =
          derrick.currentLift > kLiftStepPerTick ? derrick.currentLift - kLiftStepPerTick : 0;
      derrick.currentLift = max<uint8_t>(derrick.targetLift, nextLift);
    }

    writeServoOutputs(i);
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
  status.clusterId = kClusterId;
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
  if (packet.clusterId != 0 && packet.clusterId != kClusterId) {
    return;
  }

  gLastSequence = packet.sequence;
  gLastCommandAtMs = nowMs;
  gActiveDerricks = packet.activeDerricks > kMaxDerricks ? kMaxDerricks : packet.activeDerricks;

  if (packet.flags & kCommandFlagEmergencyHide) {
    applyFailsafeTargets();
  }

  if (packet.flags & kCommandFlagApplyOutputs) {
    for (uint8_t i = 0; i < gActiveDerricks; ++i) {
      gDerricks[i].targetPan = applyPanCalibration(i, packet.derricks[i].pan);
      gDerricks[i].targetLift = applyLiftCalibration(i, packet.derricks[i].lift);
      gDerricks[i].output = packet.derricks[i];
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

  ClusterCommandPacket packet = {};
  memcpy(&packet, data, sizeof(packet));
  ensureControllerPeer(senderMac);
  handleCommand(packet, millis());
}

void onEspNowSend(const uint8_t* macAddr, esp_now_send_status_t status) {
  (void)macAddr;
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
    gDerricks[i].calibration = {10, 90, 170, 0, 170};
    gDerricks[i].currentPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].targetPan = gDerricks[i].calibration.panCenter;
    gDerricks[i].currentLift = gDerricks[i].calibration.liftMin;
    gDerricks[i].targetLift = gDerricks[i].calibration.liftMin;
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
  bool anyServoChannels = false;
  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    if (kServoPanChannels[i] >= 0 || kServoLiftChannels[i] >= 0) {
      anyServoChannels = true;
      break;
    }
  }

  gServosReady = anyServoChannels;
  gLedsReady = kLedDataPin >= 0;
  gAudio1Ready = kDfPlayer1RxPin >= 0 && kDfPlayer1TxPin >= 0;
  gAudio2Ready = kDfPlayer2RxPin >= 0 && kDfPlayer2TxPin >= 0;
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

  initializeDerrickState();
  initializePirInputs();
  initializeHardwareAvailability();

  if (!initializeEspNow()) {
    logLine("ESP-NOW init failed.");
    return;
  }

  logLine("Cluster controller ready.");
}

void loop() {
  const uint32_t nowMs = millis();

  updatePirInputs();

  if (gLastCommandAtMs != 0 && (nowMs - gLastCommandAtMs) > kCommandTimeoutMs) {
    applyFailsafeTargets();
  }

  updateMotion(nowMs);
  updateAudioOutputs();
  publishIfNeeded(nowMs);
}
