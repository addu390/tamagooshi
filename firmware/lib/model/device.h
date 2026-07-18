#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "buddy.h"
#include "mascot.h"
#include "metric.h"
#include "pager.h"
#include "voice.h"

namespace tama {

enum class Orientation { Portrait, Landscape };

inline int rotationFor(Orientation o) { return o == Orientation::Landscape ? 1 : 0; }

struct JoystickEvent {
  int x = 0;
  int y = 0;
  bool pressed = false;
};

struct GestureEvent {
  std::string name;
};

struct MotionEvent {
  float ax = 0.0f;
  float ay = 0.0f;
  float az = 0.0f;
  std::string event;
};

struct DeviceCapabilities {
  std::string model;
  int screenW = 0;
  int screenH = 0;
  int buttons = 0;
  std::string led = "none";
  bool buzzer = false;
  bool speaker = false;
  bool mic = false;
  bool imu = false;
  bool joystick = false;
  bool haptics = false;
  bool ir = false;
  bool wearable = false;
  bool psram = false;
};

struct Branding {
  std::string name = "TAMAGOOSHI";
  std::string tagline;
  std::string website;
  std::string logo_id;
  std::string mascot_name;
  std::string dev_name;
};

struct Enabled {
  std::vector<std::string> games;
  std::vector<std::string> apps;
  std::vector<std::string> packs;
  std::vector<std::string> themes;
  std::vector<std::string> typefaces;

  bool game(const char* id) const { return has(games, id); }
  bool app(const char* id) const { return has(apps, id); }
  bool pack(const char* id) const { return has(packs, id); }
  bool theme(const char* id) const { return has(themes, id); }
  bool typeface(const char* id) const { return has(typefaces, id); }

  using NameFn = const char* (*)(int);
  int nextTheme(int count, int cur, NameFn name) const { return next(themes, count, cur, name); }
  int nextTypeface(int count, int cur, NameFn name) const {
    return next(typefaces, count, cur, name);
  }

 private:
  static bool has(const std::vector<std::string>& list, const char* id) {
    if (list.empty()) return true;
    if (!id) return false;
    for (const std::string& entry : list) {
      if (entry == id) return true;
    }
    return false;
  }

  static int next(const std::vector<std::string>& list, int count, int cur, NameFn name) {
    if (count <= 0) return cur;
    for (int step = 1; step <= count; ++step) {
      const int i = (cur + step) % count;
      if (has(list, name(i))) return i;
    }
    return cur;
  }
};

struct SysInfo {
  std::string fw;
  std::string id;
  uint32_t flash_kb = 0;
  uint32_t heap_total_kb = 0;
};

struct ConfigPatch {
  std::optional<uint32_t> carousel_secs;
  std::optional<bool> buzzer_enabled;
  std::string theme;
  std::string character_id;
};

struct DeviceState {
  Branding branding;
  Enabled enabled;
  SysInfo sysinfo;
  std::vector<Metric> metrics;
  std::optional<Page> prompt;
  BuddyState buddy;
  VoiceChat voice;
  AgentRoster agents;
  Mood mood = Mood::Neutral;
  std::string mood_reason;
  std::string character_id;
  Orientation orientation = Orientation::Portrait;
  uint32_t carousel_secs = 5;
  int16_t tz_offset_min = 0;
  uint8_t brightness = 120;
  int batt_pct = 100;
  uint32_t free_heap = 0;
  uint32_t up_secs = 0;
  bool charging = false;
  bool buzzer_enabled = true;
  bool muted = false;
  bool stale = false;
  bool voice_active = false;
  bool connected = false;
  bool hub_connected = false;
  bool agent_connected = false;
  bool dirty = false;

  void upsertMetric(const Metric& m);
  const Metric* starMetric() const;
  const Metric* metricByKey(const std::string& key) const;

  void raisePrompt(const Page& page);
  void clearPrompt(const std::string& id);
  void clearPromptSource(const std::string& source);

 private:
  std::vector<Page> pending_;
  void selectPrompt();
};

}  // namespace tama
