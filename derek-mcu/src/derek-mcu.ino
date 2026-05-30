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
constexpr uint8_t kMaxClusters = 15;
constexpr uint8_t kMaxDerricks = 8;
constexpr uint32_t kDiscoveryIntervalMs = 2000;
constexpr uint32_t kCommandIntervalMs = 100;
constexpr uint32_t kStatusPageIntervalMs = 1000;
constexpr uint32_t kClusterOfflineMs = 3000;
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// OLED and buttons are intentionally unassigned until the actual hardware is known.
constexpr int8_t kOledSdaPin = -1;
constexpr int8_t kOledSclPin = -1;
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
  uint8_t lastPirBits;
  uint16_t lastHealthFlags;
  uint32_t lastSeenAtMs;
  uint32_t lastStatusUptimeMs;
  ClusterCommandPacket desiredCommand;
};

ClusterRuntime gClusters[kMaxClusters];
uint32_t gLastDiscoveryAtMs = 0;
uint32_t gLastCommandAtMs = 0;
uint32_t gLastStatusPageAtMs = 0;
uint16_t gSequenceCounter = 1;
uint8_t gCurrentPage = 0;

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

bool isProtocolPacketValid(const uint8_t* data, size_t len) {
  if (len < sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t)) {
    return false;
  }

  const uint16_t magic = static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
  return magic == kProtocolMagic && data[2] == kProtocolVersion;
}

void initializeDefaultCommand(ClusterCommandPacket& command, uint8_t clusterId) {
  memset(&command, 0, sizeof(command));
  command.magic = kProtocolMagic;
  command.version = kProtocolVersion;
  command.kind = kMessageCommand;
  command.clusterId = clusterId;
  command.activeDerricks = kMaxDerricks;
  command.flags = kCommandFlagApplyOutputs;

  for (uint8_t i = 0; i < kMaxDerricks; ++i) {
    command.derricks[i].pan = 90;
    command.derricks[i].lift = 0;
  }
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
  cluster.lastPirBits = status.pirStateBits;
  cluster.lastHealthFlags = status.healthFlags;
  cluster.lastSeenAtMs = nowMs;
  cluster.lastStatusUptimeMs = status.uptimeMs;
  cluster.desiredCommand.clusterId = status.clusterId;
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
  (void)nowMs;
  // TODO: Replace this serial status page with OLED rendering once display hardware is selected.
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

void sendCommandToCluster(ClusterRuntime& cluster) {
  cluster.desiredCommand.magic = kProtocolMagic;
  cluster.desiredCommand.version = kProtocolVersion;
  cluster.desiredCommand.kind = kMessageCommand;
  cluster.desiredCommand.sequence = gSequenceCounter++;

  esp_now_send(
      cluster.mac,
      reinterpret_cast<const uint8_t*>(&cluster.desiredCommand),
      sizeof(cluster.desiredCommand));

  cluster.desiredCommand.flags &= kCommandFlagApplyOutputs;
  cluster.desiredCommand.audioTrackA = 0;
  cluster.desiredCommand.audioTrackB = 0;
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

  Serial.println("Commands: list | hide <cluster> | track <cluster> <derrick> <pan> <lift> | eyes <cluster> <derrick> <r> <g> <b>");
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

}  // namespace

void setup() {
  if (kEnableSerialLogs) {
    Serial.begin(115200);
  }

  memset(gClusters, 0, sizeof(gClusters));
  updateButtonInputs();

  if (!initializeEspNow()) {
    Serial.println("ESP-NOW init failed.");
    return;
  }

  Serial.println("MCU gateway ready.");
  Serial.println("Serial console commands: list, hide, track, eyes");
}

void loop() {
  const uint32_t nowMs = millis();

  updateSerialConsole();
  markOfflineClusters(nowMs);

  if ((nowMs - gLastDiscoveryAtMs) >= kDiscoveryIntervalMs) {
    broadcastDiscovery();
    gLastDiscoveryAtMs = nowMs;
  }

  sendScheduledCommands(nowMs);

  if ((nowMs - gLastStatusPageAtMs) >= kStatusPageIntervalMs) {
    renderStatusPage(nowMs);
    gLastStatusPageAtMs = nowMs;
    gCurrentPage = (gCurrentPage + 1) % 2;
  }
}
