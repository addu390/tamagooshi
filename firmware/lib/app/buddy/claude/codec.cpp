#include "codec.h"

#include <ArduinoJson.h>

namespace tama {

namespace {

std::string str(JsonVariantConst v) { return v.is<const char*>() ? v.as<const char*>() : ""; }

std::string joinTurnText(JsonArrayConst content) {
  std::string out;
  for (JsonVariantConst block : content) {
    if (str(block["type"]) != "text") continue;
    const std::string text = str(block["text"]);
    if (text.empty()) continue;
    if (!out.empty()) out.push_back(' ');
    out += text;
  }
  return out;
}

void readSnapshot(JsonDocument& doc, BuddySnapshot& snap) {
  snap.total = doc["total"] | 0;
  snap.running = doc["running"] | 0;
  snap.waiting = doc["waiting"] | 0;
  snap.msg = str(doc["msg"]);
  snap.tokens = doc["tokens"] | 0;
  snap.tokens_today = doc["tokens_today"] | 0;
  for (JsonVariantConst entry : doc["entries"].as<JsonArrayConst>()) {
    snap.entries.push_back(str(entry));
  }
  JsonVariantConst prompt = doc["prompt"];
  if (!prompt.isNull()) {
    snap.has_prompt = true;
    snap.prompt_id = str(prompt["id"]);
    snap.prompt_tool = str(prompt["tool"]);
    snap.prompt_hint = str(prompt["hint"]);
  }
}

}  // namespace

bool ClaudeCodec::decode(const std::string& line, ClaudeInbound& out) const {
  JsonDocument doc;
  if (deserializeJson(doc, line) != DeserializationError::Ok) return false;

  if (str(doc["evt"]) == "turn") {
    out.kind = ClaudeMsg::Turn;
    out.turn_text = joinTurnText(doc["content"].as<JsonArrayConst>());
    return true;
  }

  JsonVariantConst time = doc["time"];
  if (time.is<JsonArrayConst>()) {
    out.kind = ClaudeMsg::Time;
    out.epoch = time[0] | 0;
    out.tz_offset = time[1] | 0;
    return true;
  }

  const std::string cmd = str(doc["cmd"]);
  if (!cmd.empty()) {
    out.name = str(doc["name"]);
    if (cmd == "owner") {
      out.kind = ClaudeMsg::Owner;
      out.owner = out.name;
    } else {
      out.kind = ClaudeMsg::Command;
      out.command = cmd;
    }
    return true;
  }

  if (!doc["total"].isNull() || !doc["running"].isNull() || !doc["msg"].isNull()) {
    out.kind = ClaudeMsg::Snapshot;
    readSnapshot(doc, out.snapshot);
    return true;
  }

  return false;
}

std::string ClaudeCodec::encodePermission(const std::string& id, bool allow) const {
  JsonDocument doc;
  doc["cmd"] = "permission";
  doc["id"] = id;
  doc["decision"] = allow ? "once" : "deny";
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string ClaudeCodec::encodeAck(const std::string& cmd, bool ok, long n) const {
  JsonDocument doc;
  doc["ack"] = cmd;
  doc["ok"] = ok;
  doc["n"] = n;
  std::string out;
  serializeJson(doc, out);
  return out;
}

std::string ClaudeCodec::encodeStatus(const ClaudeStatus& status) const {
  JsonDocument doc;
  doc["ack"] = "status";
  doc["ok"] = true;
  JsonObject data = doc["data"].to<JsonObject>();
  data["name"] = status.name;
  data["sec"] = status.secured;
  JsonObject bat = data["bat"].to<JsonObject>();
  bat["pct"] = status.batt_pct;
  bat["mV"] = status.mV;
  bat["mA"] = status.mA;
  bat["usb"] = status.usb;
  JsonObject sys = data["sys"].to<JsonObject>();
  sys["up"] = status.up_secs;
  sys["heap"] = status.heap;
  JsonObject stats = data["stats"].to<JsonObject>();
  stats["appr"] = status.approvals;
  stats["deny"] = status.denials;
  std::string out;
  serializeJson(doc, out);
  return out;
}

}  // namespace tama
