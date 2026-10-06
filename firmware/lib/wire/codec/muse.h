#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "model.h"

namespace tama::muse {

struct Param {
  const char* name;
  const char* type;
  const char* description;
  bool required;
};

struct CommandSpec {
  const char* name;
  const char* description;
  const Param* params;
  size_t paramCount;
  uint32_t timeoutMs;
};

struct Registration {
  std::string node;
  std::string name;
  std::string version;
  std::string ssid;
  std::string commands;
};

enum class ControlKind { Ignored, Invoke, Registered, Rejected, Unpaired };

struct Control {
  ControlKind kind = ControlKind::Ignored;
  std::string id;
  bool heartbeats = false;
};

struct Invoke {
  std::string command;
  std::string params = "{}";
};

struct Vm {
  std::string id;
  std::string token;
};

struct Device {
  std::string node;
  std::string id;
  std::string mac;
  std::string firmware;
};

struct Hello {
  std::string pub;
  std::string nonce;
};

struct Ready {
  std::string pub;
  std::string nonce;
  std::string transcript;
  std::string session;
};

struct Sealed {
  std::string session;
  uint64_t counter = 0;
  std::string ciphertext;
  std::string tag;
};

struct Provision {
  std::string ssid;
  std::string password;
  Account account;
};

struct Ack {
  std::string message;
  std::string parent;
};

struct Row {
  std::string event;
  uint64_t seq = 0;
  std::string message;
  std::string replyTo;
  std::string text;
  bool ready = true;
};

inline constexpr int kPairingVersion = 5;
inline constexpr const char* kPairingModel = "hatch_link";
inline constexpr const char* kPairingAuth = "none";
inline constexpr const char* kPairingPolicy = "confirm_press";
inline constexpr const char* kPairingSuite = "p256-hkdf-sha256-aes-gcm-v1";
inline constexpr int kConfirmTimeoutSeconds = 60;

std::string encodeManifest(const CommandSpec* specs, size_t count);
std::string encodeRegister(const std::string& id, const Registration& reg);
std::string encodeHeartbeat();
std::string encodeResult(const std::string& id, const std::string& result);
Control decodeControl(const std::string& message, const std::string& registerId);
std::string decodeIdentity(const std::string& body);

bool decodeInvoke(const std::string& message, Invoke& out);
std::string missingParam(const CommandSpec& spec, const std::string& params);
std::string param(const std::string& params, const char* name);
std::string encodeSuccess(const std::string& payload = "{}");
std::string encodeFailure(const std::string& error);
std::string encodeOutcome(const char* outcome);
std::string encodeDescription(const DeviceState& state);
std::string encodeAlertPage(const std::string& params, const std::string& id, const char* source);

bool decodeVm(const std::string& body, Vm& out);
std::string encodeTokenRequest(const std::string& node, const std::string& sdkToken);
bool decodeTokens(const std::string& body, Account& account);

std::string decodeAction(const std::string& command);
bool isClientFinished(const std::string& command);
bool decodeHello(const std::string& command, Hello& out);
bool decodeSealed(const std::string& command, Sealed& out);
bool decodeProvision(const std::string& command, Provision& out);
std::string encodeReady(const Device& device, const Ready& ready);
std::string encodeSealed(const Sealed& sealed);
std::string encodeStatus(const char* status, const std::string& sdkToken = "");
std::string encodeDeviceInfo(const Device& device);
std::string encodeScan(const std::vector<std::string>& ssids);

std::string encodeNoteHead(const std::string& device);
std::string encodeNoteTail();
bool decodeAck(const std::string& body, Ack& out);
bool decodeRow(const std::string& line, Row& out);
bool decodeClientInvoke(const std::string& line, std::string& id, Invoke& out);

}  // namespace tama::muse
