#include "muse.h"

#include <ArduinoJson.h>

#include <cctype>

#include "fields.h"

namespace tama::muse {

namespace {

constexpr char kRegister[] = "link.register";
constexpr char kHeartbeat[] = "link.heartbeat";
constexpr char kResult[] = "link.result";
constexpr char kInvoke[] = "link.invoke";

std::string serialize(const JsonDocument& doc) {
  std::string out;
  serializeJson(doc, out);
  return out;
}

bool parse(const std::string& json, JsonDocument& doc) {
  return deserializeJson(doc, json) == DeserializationError::Ok;
}

bool truthy(JsonVariantConst v) { return !v.isNull() && !(v.is<bool>() && !v.as<bool>()); }

std::string hostLabel(const std::string& url) {
  const size_t scheme = url.find("://");
  if (scheme == std::string::npos) return {};
  const size_t start = scheme + 3;
  const size_t dot = url.find('.', start);
  if (dot == std::string::npos || dot == start) return {};
  const std::string label = url.substr(start, dot - start);
  for (char c : label) {
    if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-') return {};
  }
  return label;
}

}  // namespace

std::string encodeManifest(const CommandSpec* specs, size_t count) {
  JsonDocument doc;
  doc.to<JsonObject>();
  for (size_t i = 0; i < count; ++i) {
    const CommandSpec& spec = specs[i];
    JsonObject entry = doc[spec.name].to<JsonObject>();
    entry["description"] = spec.description;
    JsonObject required = entry["required"].to<JsonObject>();
    JsonObject optional = entry["optional"].to<JsonObject>();
    for (size_t p = 0; p < spec.paramCount; ++p) {
      const Param& param = spec.params[p];
      JsonObject schema = (param.required ? required : optional)[param.name].to<JsonObject>();
      schema["type"] = param.type;
      schema["description"] = param.description;
    }
    if (spec.timeoutMs) entry["timeout_ms"] = spec.timeoutMs;
  }
  return serialize(doc);
}

std::string encodeRegister(const std::string& id, const Registration& reg) {
  JsonDocument doc;
  doc["type"] = "req";
  doc["id"] = id;
  doc["method"] = kRegister;
  JsonObject params = doc["params"].to<JsonObject>();
  params["node_id"] = reg.node;
  params["display_name"] = reg.name;
  params["platform"] = "esp32";
  params["version"] = reg.version;
  params["device_family"] = "homehub";
  params["model_id"] = "esp32";
  params["is_wakeup_supported"] = false;
  if (!reg.ssid.empty()) params["metadata"]["network_ssid"] = reg.ssid;
  params["commands_v2"] = serialized(reg.commands);
  return serialize(doc);
}

std::string encodeHeartbeat() {
  JsonDocument doc;
  doc["method"] = kHeartbeat;
  return serialize(doc);
}

std::string encodeResult(const std::string& id, const std::string& result) {
  JsonDocument doc;
  if (!parse(result, doc)) doc.clear();
  doc["method"] = kResult;
  doc["id"] = id;
  return serialize(doc);
}

Control decodeControl(const std::string& message, const std::string& registerId) {
  Control out;
  JsonDocument doc;
  if (!parse(message, doc)) return out;

  const std::string type = doc["type"] | "";
  if (type == "event") {
    const std::string event = doc["event"] | "";
    if (event == "link.unpaired" || event == "node.unpaired") out.kind = ControlKind::Unpaired;
    return out;
  }

  out.id = doc["id"] | "";
  const std::string method = doc["method"] | "";
  if (method.empty() && !registerId.empty() && out.id == registerId) {
    if (truthy(doc["error"])) {
      out.kind = ControlKind::Rejected;
    } else {
      out.kind = ControlKind::Registered;
      out.heartbeats = type == "res" && std::string(doc["result"]["status"] | "") == "registered";
    }
  } else if (method == kInvoke && !out.id.empty()) {
    out.kind = ControlKind::Invoke;
  }
  return out;
}

std::string decodeIdentity(const std::string& body) {
  JsonDocument doc;
  if (!parse(body, doc)) return {};
  return doc["result"]["name"] | "";
}

bool decodeInvoke(const std::string& message, Invoke& out) {
  JsonDocument doc;
  if (!parse(message, doc)) return false;
  out.command = doc["command"] | "";
  JsonObjectConst params = doc["params"].as<JsonObjectConst>();
  if (!params.isNull()) {
    out.params.clear();
    serializeJson(params, out.params);
  }
  return !out.command.empty();
}

std::string missingParam(const CommandSpec& spec, const std::string& params) {
  JsonDocument doc;
  parse(params, doc);
  for (size_t i = 0; i < spec.paramCount; ++i) {
    const Param& p = spec.params[i];
    const char* value = doc[p.name] | "";
    if (p.required && !*value) return p.name;
  }
  return {};
}

std::string param(const std::string& params, const char* name) {
  JsonDocument doc;
  if (!parse(params, doc)) return {};
  return doc[name] | "";
}

std::string encodeSuccess(const std::string& payload) {
  JsonDocument doc;
  doc["ok"] = true;
  doc["payload"] = serialized(payload);
  return serialize(doc);
}

std::string encodeFailure(const std::string& error) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = error;
  return serialize(doc);
}

std::string encodeOutcome(const char* outcome) {
  JsonDocument doc;
  doc["outcome"] = outcome;
  return serialize(doc);
}

std::string encodeDescription(const DeviceState& state) {
  JsonDocument doc;
  doc["name"] = state.branding.mascot_name;
  doc["mood"] = moodToString(state.mood);
  if (!state.mood_reason.empty()) doc["reason"] = state.mood_reason;
  doc["battery"] = state.batt_pct;
  doc["charging"] = state.charging;
  doc["alerting"] = state.prompt.has_value();
  JsonArray metrics = doc["metrics"].to<JsonArray>();
  for (const Metric& metric : state.metrics) {
    JsonObject entry = metrics.add<JsonObject>();
    entry[fields::kKey] = metric.key;
    entry[fields::kLabel] = metric.label;
    entry[fields::kValue] = metric.value;
    if (!metric.trend.empty()) entry[fields::kTrend] = metric.trend;
  }
  return serialize(doc);
}

std::string encodeAlertPage(const std::string& params, const std::string& id,
                            const char* source) {
  JsonDocument doc;
  parse(params, doc);
  doc[fields::kId] = id;
  doc[fields::kSource] = source;
  const char* severity = doc[fields::kSeverity] | "";
  if (!*severity) doc[fields::kSeverity] = severityToString(Severity::Warning);
  return serialize(doc);
}

bool decodeVm(const std::string& body, Vm& out) {
  JsonDocument doc;
  if (!parse(body, doc) || !doc["error_title"].isNull() || !doc["backend_error_code"].isNull())
    return false;

  JsonObjectConst chosen;
  for (JsonObjectConst vm : doc["vm_list"].as<JsonArrayConst>()) {
    const char* token = vm["vm_auth_token"] | "";
    if (!*token) continue;
    const bool preferred = vm["default"] | false;
    if (chosen.isNull() || preferred) chosen = vm;
    if (preferred) break;
  }
  if (chosen.isNull()) return false;

  out.token = chosen["vm_auth_token"].as<const char*>();
  out.id = chosen["vm_id"] | "";
  if (out.id.empty()) out.id = hostLabel(chosen["vm_ws_url"] | (chosen["vm_url"] | ""));
  return !out.id.empty();
}

std::string encodeTokenRequest(const std::string& node, const std::string& sdkToken) {
  JsonDocument doc;
  doc["device_id"] = node;
  if (!sdkToken.empty()) doc["sdk_token"] = sdkToken;
  return serialize(doc);
}

bool decodeTokens(const std::string& body, Account& account) {
  JsonDocument doc;
  if (!parse(body, doc)) return false;
  const char* access = doc["access_token"] | "";
  const char* refresh = doc["refresh_token"] | "";
  if (!*access || !*refresh) return false;
  account.access = access;
  account.refresh = refresh;
  return true;
}

std::string decodeAction(const std::string& command) {
  JsonDocument doc;
  if (!parse(command, doc)) return {};
  return doc["action"] | "";
}

bool isClientFinished(const std::string& command) {
  JsonDocument doc;
  if (!parse(command, doc)) return false;
  JsonObjectConst root = doc.as<JsonObjectConst>();
  return root.size() == 1 && std::string(root["action"] | "") == "pairing_client_finished";
}

bool decodeHello(const std::string& command, Hello& out) {
  JsonDocument doc;
  if (!parse(command, doc)) return false;
  if ((doc["version"] | 0) != kPairingVersion) return false;
  if (std::string(doc["pairing_auth"] | "") != kPairingAuth) return false;
  if (std::string(doc["pairing_policy"] | "") != kPairingPolicy) return false;
  out.pub = doc["mobile_pub"] | "";
  out.nonce = doc["mobile_nonce"] | "";
  return !out.pub.empty() && !out.nonce.empty();
}

bool decodeSealed(const std::string& command, Sealed& out) {
  JsonDocument doc;
  if (!parse(command, doc)) return false;
  out.session = doc["session_id"] | "";
  out.ciphertext = doc["ciphertext"] | "";
  out.tag = doc["tag"] | "";
  const std::string counter = doc["counter"] | "";
  if (counter.empty() || counter.size() > 19) return false;
  out.counter = 0;
  for (char c : counter) {
    if (c < '0' || c > '9') return false;
    out.counter = out.counter * 10 + static_cast<uint64_t>(c - '0');
  }
  return !out.session.empty() && !out.ciphertext.empty() && !out.tag.empty();
}

bool decodeProvision(const std::string& command, Provision& out) {
  JsonDocument doc;
  if (!parse(command, doc)) return false;
  out.ssid = doc["ssid"] | "";
  out.password = doc["password"] | "";
  out.account.access = doc["access_token"] | "";
  out.account.refresh = doc["refresh_token"] | "";
  out.account.api = doc["api_url_v2"] | "";
  out.account.noise = doc["noise_host"] | "";
  return !out.ssid.empty() && !out.password.empty() && out.account.valid() &&
         std::string(doc["token_type"] | "") == "device";
}

std::string encodeReady(const Device& device, const Ready& ready) {
  JsonDocument doc;
  doc["type"] = "pairing_ready";
  doc["version"] = kPairingVersion;
  doc["device_id"] = device.id;
  doc["node_id"] = device.node;
  doc["mac"] = device.mac;
  doc["model"] = kPairingModel;
  doc["firmware_version"] = device.firmware;
  doc["pairing_auth"] = kPairingAuth;
  doc["pairing_auth_epoch"] = 0;
  doc["pairing_policy"] = kPairingPolicy;
  doc["device_pub"] = ready.pub;
  doc["device_nonce"] = ready.nonce;
  doc["transcript_hash"] = ready.transcript;
  doc["session_id"] = ready.session;
  return serialize(doc);
}

std::string encodeSealed(const Sealed& sealed) {
  JsonDocument doc;
  doc["type"] = "pairing_encrypted";
  doc["session_id"] = sealed.session;
  doc["counter"] = std::to_string(sealed.counter);
  doc["ciphertext"] = sealed.ciphertext;
  doc["tag"] = sealed.tag;
  return serialize(doc);
}

std::string encodeStatus(const char* status, const std::string& sdkToken) {
  JsonDocument doc;
  doc["type"] = "status";
  doc["status"] = status;
  if (!sdkToken.empty()) doc["sdk_token"] = sdkToken;
  return serialize(doc);
}

std::string encodeDeviceInfo(const Device& device) {
  JsonDocument doc;
  doc["type"] = "device_info";
  doc["node_id"] = device.node;
  doc["version"] = device.firmware;
  doc["device_id"] = device.id;
  doc["mac"] = device.mac;
  doc["model"] = kPairingModel;
  doc["pairing_protocol"] = kPairingVersion;
  doc["pairing_auth"] = kPairingAuth;
  doc["pairing_auth_epoch"] = 0;
  doc["pairing_policy"] = kPairingPolicy;
  return serialize(doc);
}

std::string encodeScan(const std::vector<std::string>& ssids) {
  JsonDocument doc;
  doc["type"] = "wifi_scan_result";
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (const std::string& ssid : ssids) {
    JsonObject entry = networks.add<JsonObject>();
    entry["ssid"] = ssid;
    entry["rssi"] = 0;
    entry["secure"] = true;
  }
  return serialize(doc);
}

std::string encodeNoteHead(const std::string& device) {
  JsonDocument doc;
  doc["message"] = "";
  doc["output_modality"] = "text";
  doc["device_id"] = device;
  std::string head = serialize(doc);
  head.pop_back();
  return head +
         ",\"items\":[{\"type\":\"file\",\"mime_type\":\"audio/wav\","
         "\"filename\":\"voice_note.wav\",\"data_base64\":\"";
}

std::string encodeNoteTail() { return "\"}]}"; }

bool decodeAck(const std::string& body, Ack& out) {
  JsonDocument doc;
  if (!parse(body, doc)) return false;
  JsonVariantConst root = doc.as<JsonVariantConst>();
  JsonVariantConst result = root["result"].is<JsonObjectConst>() ? root["result"] : root;
  out.message = result["message_id"] | "";
  out.parent = result["reply_to_message_id"] | "";
  return !out.message.empty();
}

bool decodeRow(const std::string& line, Row& out) {
  JsonDocument doc;
  if (!parse(line, doc) || std::string(doc["type"] | "") != "event") return false;
  JsonVariantConst root = doc.as<JsonVariantConst>();
  JsonVariantConst payload = root["payload"];
  auto field = [&](const char* key) -> JsonVariantConst {
    JsonVariantConst value = payload[key];
    return value.isNull() ? root[key] : value;
  };
  auto text = [&](const char* key) -> std::string { return field(key) | ""; };

  out = Row{};
  out.seq = doc["seq"].as<uint64_t>();
  out.event = root["event"] | (payload["event_name"] | (payload["event"] | ""));
  out.message = text("message_id");
  if (out.message.empty()) out.message = text("id");
  out.replyTo = text("reply_to_message_id");
  if (out.replyTo.empty()) out.replyTo = text("parent_message_id");
  for (const char* key : {"display_text", "content", "text"}) {
    out.text = text(key);
    if (!out.text.empty()) break;
  }
  JsonVariantConst ready = field("display_text_ready");
  out.ready = ready.isNull() || ready.as<bool>();
  return true;
}

bool decodeClientInvoke(const std::string& line, std::string& id, Invoke& out) {
  JsonDocument doc;
  if (!parse(line, doc) || std::string(doc["event"] | "") != "client.invoke") return false;
  JsonVariantConst payload = doc["payload"];
  id = payload["invoke_id"] | "";
  out.command = payload["command_id"] | "";
  out.params = payload["params_json"] | "{}";
  return !id.empty() && !out.command.empty();
}

}  // namespace tama::muse
