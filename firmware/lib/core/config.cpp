#include "config.h"

#include <ArduinoJson.h>

#include <cstring>

#include "logos.h"
#include "theme.h"
#include "typeface.h"

namespace tama::config {

namespace {

void readStrings(JsonVariantConst arr, std::vector<std::string>& out) {
  if (!arr.is<JsonArrayConst>()) return;
  out.clear();
  for (JsonVariantConst v : arr.as<JsonArrayConst>()) {
    const char* s = v.as<const char*>();
    if (s && *s) out.emplace_back(s);
  }
}

void assign(JsonVariantConst v, std::string& out) {
  const char* s = v.as<const char*>();
  if (s && *s) out = s;
}

bool hexNibble(char c, int& v) {
  if (c >= '0' && c <= '9') { v = c - '0'; return true; }
  if (c >= 'a' && c <= 'f') { v = c - 'a' + 10; return true; }
  if (c >= 'A' && c <= 'F') { v = c - 'A' + 10; return true; }
  return false;
}

bool hexDecode(const char* s, std::vector<uint8_t>& out) {
  out.clear();
  for (; s[0] && s[1]; s += 2) {
    int hi, lo;
    if (!hexNibble(s[0], hi) || !hexNibble(s[1], lo)) return false;
    out.push_back(static_cast<uint8_t>((hi << 4) | lo));
  }
  return s[0] == '\0';
}

void applyThemes(JsonVariantConst themes) {
  if (!themes.is<JsonArrayConst>()) return;
  for (JsonVariantConst t : themes.as<JsonArrayConst>()) {
    const char* name = t["name"].as<const char*>();
    JsonVariantConst colors = t["colors"];
    if (!name || !colors.is<JsonArrayConst>() ||
        colors.as<JsonArrayConst>().size() != TAMA_THEME_ROLE_COUNT) {
      continue;
    }

    uint16_t roles[TAMA_THEME_ROLE_COUNT];
    for (size_t i = 0; i < TAMA_THEME_ROLE_COUNT; ++i) roles[i] = colors[i].as<uint16_t>();
    theme::addRuntime(name, roles);
  }
}

bool readCondition(JsonVariantConst v, Condition& out) {
  const char* metric = v["metric"].as<const char*>();
  const char* op = v["op"].as<const char*>();
  if (!metric || !*metric || !op || !ruleOpFromString(op, out.op) || !v["value"].is<double>())
    return false;
  out.metric = metric;
  out.value = v["value"].as<double>();
  return true;
}

void readRules(JsonVariantConst doc, Reactions& out) {
  if (doc["moods"].is<JsonArrayConst>()) {
    out.moods.clear();
    for (JsonVariantConst r : doc["moods"].as<JsonArrayConst>()) {
      MoodRule rule;
      const char* mood = r["mood"].as<const char*>();
      if (!mood || !readCondition(r["when"], rule.when)) continue;
      rule.mood = moodFromString(mood);
      rule.priority = r["priority"] | 0;
      out.moods.push_back(std::move(rule));
    }
  }

  if (doc["alerts"].is<JsonArrayConst>()) {
    out.alerts.clear();
    for (JsonVariantConst r : doc["alerts"].as<JsonArrayConst>()) {
      AlertRule rule;
      const char* id = r["id"].as<const char*>();
      const char* title = r["title"].as<const char*>();
      if (!id || !*id || !title || !readCondition(r["when"], rule.when)) continue;
      rule.id = id;
      rule.title = title;
      assign(r["body"], rule.body);
      assign(r["source"], rule.source);
      if (const char* s = r["severity"].as<const char*>()) rule.severity = severityFromString(s);
      rule.requires_ack = r["requires_ack"] | true;
      out.alerts.push_back(std::move(rule));
    }
  }
}

void applyLogo(JsonVariantConst logo, DeviceState& state) {
  if (!logo.is<JsonObjectConst>()) return;
  const int w = logo["w"] | 0;
  const int h = logo["h"] | 0;
  const char* bits = logo["bits"].as<const char*>();
  if (w <= 0 || h <= 0 || !bits) return;
  std::vector<uint8_t> packed;
  if (!hexDecode(bits, packed)) return;
  if (state.branding.logo_id.empty()) state.branding.logo_id = "brand";
  logos::setRuntime(state.branding.logo_id, w, h, packed);
}

}  // namespace

bool apply(const std::string& blob, DeviceState& state) {
  if (blob.size() < kHeaderSize) return false;
  if (std::memcmp(blob.data(), kMagic, sizeof(kMagic)) != 0) return false;

  const size_t len = static_cast<uint8_t>(blob[4]) | (static_cast<uint8_t>(blob[5]) << 8);
  if (len == 0 || kHeaderSize + len > blob.size()) return false;

  JsonDocument doc;
  if (deserializeJson(doc, blob.data() + kHeaderSize, len) != DeserializationError::Ok) return false;

  JsonVariantConst brand = doc["brand"];
  assign(brand["name"], state.branding.name);
  assign(brand["tagline"], state.branding.tagline);
  assign(brand["website"], state.branding.website);
  assign(brand["mascot"], state.branding.mascot_name);

  applyThemes(doc["themes"]);

  JsonVariantConst defaults = doc["defaults"];
  if (const char* t = defaults["theme"].as<const char*>()) theme::setThemeByName(t);
  if (const char* f = defaults["typeface"].as<const char*>()) typeface::setTypefaceByName(f);
  assign(defaults["mascot"], state.character_id);
  if (const char* m = defaults["mood"].as<const char*>()) {
    state.reactions.fallback = moodFromString(m);
    state.setMood(state.reactions.fallback);
  }
  if (defaults["tz"].is<int>()) state.tz_offset_min = static_cast<int16_t>(defaults["tz"].as<int>());

  JsonVariantConst enabled = doc["enabled"];
  readStrings(enabled["games"], state.enabled.games);
  readStrings(enabled["apps"], state.enabled.apps);
  readStrings(enabled["packs"], state.enabled.packs);
  readStrings(enabled["themes"], state.enabled.themes);
  readStrings(enabled["typefaces"], state.enabled.typefaces);

  readRules(doc.as<JsonVariantConst>(), state.reactions);
  applyLogo(doc["logo"], state);

  return true;
}

bool applyRules(const char* json, Reactions& out) {
  JsonDocument doc;
  if (!json || deserializeJson(doc, json) != DeserializationError::Ok) return false;
  readRules(doc.as<JsonVariantConst>(), out);
  return true;
}

}  // namespace tama::config
