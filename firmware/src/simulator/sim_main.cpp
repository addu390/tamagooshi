#include <M5Unified.h>
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "adapters.h"
#include "board.gen.h"
#include "brand.gen.h"
#include <ArduinoJson.h>

#if defined(TAMA_ENABLE_BUDDY)
#include "buddy/agent/commands.h"
#include "buddy/agent/session.h"
#include "buddy/controller.h"
#include "buddy/voice/controller.h"
#include "buddy/voice/uplink.h"
#endif
#include "capture.h"
#include "hal/m5_buttons.h"
#include "hal/profile.h"
#include "hub/pipeline.h"
#include "input.h"
#include "json.h"
#include "link.h"
#include "screens.h"
#include "transport.h"
#include "mqtt.h"
#include "runtime.h"
#include "channels.h"
#include "telemetry.h"
#include "ulid.h"
#include "wifi/control.h"

using namespace tama;

namespace {

constexpr const char* kSimId = "sim";

uint64_t nowMs() { return static_cast<uint64_t>(m5gfx::millis()); }

class SimSensor : public ISensorSource {
 public:
  void begin() override {}
  void poll() override {}
  void onMotion(MotionHandler) override {}
  float tiltX() override { return read(); }
  float tiltY() override { return read(); }

  bool accel(float& ax, float& ay, float& az) override {
    int n = 0;
    const Uint8* ks = SDL_GetKeyboardState(&n);
    ax = ay = 0.0f;
    if (ks) {
      if (ks[SDL_SCANCODE_RIGHT]) ax = 0.8f;
      if (ks[SDL_SCANCODE_LEFT]) ax = -0.8f;
      if (ks[SDL_SCANCODE_UP]) ay = 0.8f;
      if (ks[SDL_SCANCODE_DOWN]) ay = -0.8f;
    }
    az = std::sqrt(std::max(0.0f, 1.0f - ax * ax - ay * ay));
    return true;
  }

 private:
  static float read() {
    int n = 0;
    const Uint8* ks = SDL_GetKeyboardState(&n);
    if (!ks) return 0.0f;
    if (ks[SDL_SCANCODE_RIGHT]) return 0.8f;
    if (ks[SDL_SCANCODE_LEFT]) return -0.8f;
    return 0.0f;
  }
};

class SimConfig : public config::ISource {
 public:
  std::string read() override {
    const char* path = std::getenv("TAMA_CONFIG_BLOB");
    if (!path) return {};
    FILE* f = std::fopen(path, "rb");
    if (!f) return {};
    std::string out;
    char buf[512];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) out.append(buf, n);
    std::fclose(f);
    return out;
  }
};

class SimLink : public ILink {
 public:
  bool available() const override { return true; }
  bool enabled() const override { return enabled_; }
  void setEnabled(bool on) override {
    enabled_ = on;
    if (!on) paired_ = false;
  }
  bool connected() const override { return enabled_ && paired_; }
  std::string peer() const override { return paired_ ? "John's iPhone" : std::string{}; }
  std::string deviceName() const override { return "Claude-gooshi-a1b2"; }
  std::string deviceId() const override { return "sim"; }
  uint32_t passkey() const override { return 429173; }
  bool paired() const override { return paired_; }
  void unpair() override { paired_ = false; }
  void configure(bool enabled, bool paired) {
    enabled_ = enabled;
    paired_ = paired;
  }

 private:
  bool enabled_ = true;
  bool paired_ = false;
};

class SimWifi : public IWifiControl {
 public:
  bool available() const override { return true; }
  bool enabled() const override { return enabled_; }
  void setEnabled(bool on) override { enabled_ = on; }
  bool connected() const override { return enabled_ && !active_.empty() && !provisioning_; }
  std::string peer() const override { return connected() ? active_ : std::string{}; }

  std::vector<KnownNetwork> known() const override {
    std::vector<KnownNetwork> out;
    for (const auto& s : nets_) out.push_back({s, s == active_});
    return out;
  }
  void select(const std::string& ssid) override {
    active_ = ssid;
    provisioning_ = false;
  }
  void forget(const std::string& ssid) override {
    nets_.erase(std::remove(nets_.begin(), nets_.end(), ssid), nets_.end());
    if (active_ == ssid) active_ = nets_.empty() ? std::string{} : nets_.front();
  }
  void provision() override { provisioning_ = true; }
  bool provisioning() const override { return provisioning_; }
  std::string portal() const override { return "gooshi-setup"; }

  void configure(bool enabled, bool provisioning, std::vector<std::string> nets,
                 std::string active) {
    enabled_ = enabled;
    provisioning_ = provisioning;
    nets_ = std::move(nets);
    active_ = std::move(active);
  }

 private:
  bool enabled_ = true;
  bool provisioning_ = false;
  std::vector<std::string> nets_ = {"Home-5G", "Cafe-Guest"};
  std::string active_ = "Home-5G";
};

class SimMic : public IMicSource {
 public:
  void begin() override { active_ = true; }
  void end() override {
    active_ = false;
    level_ = 0;
  }
  int level() override {
    if (!active_) return 0;
    return loud() ? levelTo(700) : levelTo(0);
  }

  bool startRecord() override {
    if (!active_) return false;
    start_ms_ = m5gfx::millis();
    produced_ = 0;
    return true;
  }

  size_t readRecord(int16_t* out, size_t maxSamples) override {
    if (!active_) return 0;
    const uint64_t due = (m5gfx::millis() - start_ms_) * recordRate() / 1000;
    if (produced_ >= due) return 0;
    const size_t n = std::min<size_t>(maxSamples, static_cast<size_t>(due - produced_));
    const int amp = loud() ? 9000 : 500;
    for (size_t i = 0; i < n; ++i) {
      const uint64_t t = produced_ + i;
      out[i] = static_cast<int16_t>(((t * 440 * 64 / recordRate()) % 64 < 32) ? amp : -amp);
    }
    produced_ += n;
    return n;
  }

  void stopRecord() override {}

 private:
  static bool loud() {
    int n = 0;
    const Uint8* ks = SDL_GetKeyboardState(&n);
    return ks && ks[SDL_SCANCODE_UP];
  }

  int levelTo(int target) {
    level_ += (target - level_) / 3;
    return level_;
  }

  bool active_ = false;
  int level_ = 0;
  uint32_t start_ms_ = 0;
  uint64_t produced_ = 0;
};

class SimIr : public IIrTransceiver {
 public:
  void begin() override {}
  void send(const IrFrame&) override {}
  void startLearn() override { learnedAt_ = m5gfx::millis() + 2000; }
  void stopLearn() override { learnedAt_ = 0; }

  bool fetchLearned(IrFrame& out) override {
    if (learnedAt_ == 0 || m5gfx::millis() < learnedAt_) return false;
    learnedAt_ = 0;
    out = necLikeFrame();
    return true;
  }

  static IrFrame necLikeFrame() {
    IrFrame f;
    f.pulses[f.count++] = 9000;
    f.pulses[f.count++] = 4500;
    for (int i = 0; i < 16; ++i) {
      f.pulses[f.count++] = 560;
      f.pulses[f.count++] = i % 2 ? 1690 : 560;
    }
    return f;
  }

 private:
  uint32_t learnedAt_ = 0;
};

class SimIrStore : public IIrStore {
 public:
  SimIrStore() : buttons_{{"BTN 1", SimIr::necLikeFrame()}, {"BTN 2", SimIr::necLikeFrame()}} {}

  int load(IrButton* out, int max) override {
    const int n = std::min<int>(static_cast<int>(buttons_.size()), max);
    for (int i = 0; i < n; ++i) out[i] = buttons_[i];
    return n;
  }

  void save(const IrButton* buttons, int count) override {
    buttons_.assign(buttons, buttons + count);
  }

 private:
  std::vector<IrButton> buttons_;
};

#if defined(TAMA_ENABLE_BUDDY)
class SimVoiceHost : public ILineSink {
 public:
  void bind(AgentSession& session) { session_ = &session; }

  void send(const std::string& line) override {
    if (session_ == nullptr) return;
    JsonDocument doc;
    if (deserializeJson(doc, line) != DeserializationError::Ok) return;
    const std::string cmd = doc["cmd"] | "";
    if (cmd == "agents") {
      session_->onInbound("", agentsEvent());
    } else if (cmd == "voice_end") {
      const std::string agent = doc["agent"] | TAMA_HUB_AGENT_DEFAULT;
      schedule(900,
               "{\"evt\":\"transcript\",\"id\":\"sim_v1\",\"text\":\"what changed in the repo "
               "since yesterday?\",\"agent\":\"" + agent + "\"}");
    } else if (cmd == "permission" && std::string(doc["id"] | "") == "sim_v1") {
      if (std::string(doc["decision"] | "") != "once") return;
      schedule(1800,
               "{\"evt\":\"reply\",\"text\":\"Three commits landed: the board catalog "
               "gained a psram flag, \",\"done\":false}");
      schedule(2600,
               "{\"evt\":\"reply\",\"text\":\"the mic HAL grew a record API, and the "
               "docs got a voice section.\",\"done\":true}");
    }
  }

  void tick(uint32_t nowMs) {
    now_ms_ = nowMs;
    if (session_ == nullptr) return;
    while (!queue_.empty() && nowMs >= queue_.front().first) {
      const std::string payload = queue_.front().second;
      queue_.erase(queue_.begin());
      session_->onInbound("", payload);
    }
  }

 private:
  static std::string agentsEvent() {
    JsonDocument doc;
    doc["evt"] = "agents";
    JsonArray enabled = doc["enabled"].to<JsonArray>();
    const std::string joined = TAMA_HUB_AGENTS;
    size_t i = 0;
    while (i <= joined.size() && !joined.empty()) {
      const size_t comma = joined.find(',', i);
      const std::string name =
          joined.substr(i, comma == std::string::npos ? std::string::npos : comma - i);
      if (!name.empty()) enabled.add(name);
      if (comma == std::string::npos) break;
      i = comma + 1;
    }
    doc["default"] = TAMA_HUB_AGENT_DEFAULT;
    std::string out;
    serializeJson(doc, out);
    return out;
  }

  void schedule(uint32_t delayMs, std::string payload) {
    queue_.emplace_back(now_ms_ + delayMs, std::move(payload));
  }

  AgentSession* session_ = nullptr;
  uint32_t now_ms_ = 0;
  std::vector<std::pair<uint32_t, std::string>> queue_;
};
#endif  // TAMA_ENABLE_BUDDY

SimTransport g_sim;
SimTransport g_agentConn;
MqttSimTransport g_mqtt;
TransportProxy g_transport;
NullExpression g_expression;
SimLink g_simLink;
SimWifi g_simWifi;
#if defined(TAMA_ENABLE_BUDDY)
SimVoiceHost g_lineSink;
#endif
SimTelemetry g_telemetry;
M5Buttons g_buttons;
NullInputSource g_input;
SimSensor g_sensor;
SimMic g_mic;
SimIr g_ir;
SimIrStore g_irStore;
SimConfig g_config;
StaticBoardProfile g_board(board::capabilities());
IdFn g_idGen =
    ids::ulidGenerator([] { return nowMs(); }, [] { return static_cast<uint8_t>(rand()); });
ArduinoJsonCodec g_codec(g_idGen, [] { return nowMs(); }, kSimId);
SimSystemControl g_systemControl;
Runtime g_runtime(g_board.capabilities(), g_codec, g_expression, g_systemControl, g_buttons, g_input,
                  g_sensor, g_telemetry, g_mic, g_config);
HubPipeline g_hubPipeline(g_transport, g_codec, g_runtime.router(), g_board, kSimId, "0.1.0-sim");
#if defined(TAMA_ENABLE_BUDDY)
BuddyController g_buddyController(g_runtime.state());
VoiceController g_voiceController(g_runtime.state());
VoiceUplink g_voiceUplink(g_lineSink, g_runtime.state(), 15 * 8000);
AgentCommands g_agentCommands(g_simLink, g_telemetry, g_runtime.state());
AgentSession g_agentSession(g_lineSink, g_buddyController, g_agentCommands, g_voiceController,
                            g_voiceUplink);
sim::CaptureHarness g_capture(g_runtime, &g_agentSession);
#else
sim::CaptureHarness g_capture(g_runtime);
#endif

#if defined(TAMA_ENABLE_BUDDY)
void feedBuddy(const char* scenario) {
  g_agentSession.onInbound("", "{\"cmd\":\"owner\",\"name\":\"John\"}");

  const std::string s = scenario ? scenario : "working";
  if (s == "idle") {
    g_agentSession.onInbound(
        "", "{\"total\":0,\"running\":0,\"waiting\":0,\"tokens\":184502,\"tokens_today\":31200}");
  } else if (s == "waiting" || s == "approve" || s == "deny") {
    g_agentSession.onInbound(
        "",
        "{\"total\":3,\"running\":1,\"waiting\":1,\"msg\":\"approve: Bash\",\"entries\":[\"10:42 "
        "git push\"],\"tokens\":184502,\"tokens_today\":31200,\"prompt\":{\"id\":\"req_abc123\","
        "\"tool\":\"Bash\",\"hint\":\"rm -rf /tmp/foo\"}}");
  } else {
    g_agentSession.onInbound(
        "",
        "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"editing files\",\"entries\":[\"10:42 git "
        "push\",\"10:41 yarn test\",\"10:39 reading file...\"],\"tokens\":184502,\"tokens_today\":"
        "31200}");
  }
}
#endif  // TAMA_ENABLE_BUDDY

void pollKeys() {
  int n = 0;
  const Uint8* ks = SDL_GetKeyboardState(&n);
  if (!ks) return;
  static Uint8 prev[SDL_NUM_SCANCODES] = {0};
  auto pressed = [&](int sc) { return ks[sc] && !prev[sc]; };
  if (pressed(SDL_SCANCODE_RIGHT) || pressed(SDL_SCANCODE_DOWN))
    g_runtime.nav().dispatch(Intent::Next);
  if (pressed(SDL_SCANCODE_LEFT) || pressed(SDL_SCANCODE_UP))
    g_runtime.nav().dispatch(Intent::Prev);
  if (pressed(SDL_SCANCODE_RETURN) || pressed(SDL_SCANCODE_SPACE))
    g_runtime.nav().dispatch(Intent::Select);
  if (pressed(SDL_SCANCODE_BACKSPACE)) g_runtime.nav().dispatch(Intent::Back);
  if (pressed(SDL_SCANCODE_H)) g_runtime.nav().dispatch(Intent::Home);
  std::memcpy(prev, ks, n < SDL_NUM_SCANCODES ? n : SDL_NUM_SCANCODES);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  auto& sys = g_runtime.state().sysinfo;
  sys.fw = TAMA_FW_VERSION;
  sys.id = kSimId;

  if (const char* orient = std::getenv("TAMA_ORIENT")) {
    if (orient[0] == 'h' || orient[0] == 'H' || orient[0] == 'l' || orient[0] == 'L') {
      g_runtime.state().orientation = Orientation::Landscape;
    }
  }

  const char* broker = std::getenv("TAMA_BROKER");

  g_transport.bind(broker && *broker && g_mqtt.tryConnect(broker, kSimId)
                       ? static_cast<ITransport*>(&g_mqtt)
                       : static_cast<ITransport*>(&g_sim));

  auto hubResolver = makeHubResolver(g_transport, g_codec, kSimId);

  if (const char* bt = std::getenv("TAMA_BT")) {
    const std::string s = bt;
    g_simLink.configure(s != "off", s == "linked");
  }
  if (const char* wf = std::getenv("TAMA_WIFI")) {
    const std::string s = wf;
    if (s == "off") {
      g_simWifi.configure(false, false, {}, "");
    } else if (s == "portal") {
      g_simWifi.configure(true, true, {}, "");
    } else {
      g_simWifi.configure(true, false, {"Home-5G", "Cafe-Guest"}, "Home-5G");
    }
  }

  ChannelBinding binding;
  binding.link = &g_simLink;
  binding.wifi = &g_simWifi;
  binding.hub = {&g_transport, &g_hubPipeline};
#if defined(TAMA_ENABLE_BUDDY)
  auto agentResolver = makeAgentResolver(g_agentSession);
  auto voiceResolver = makeVoiceResolver(g_agentSession);
  binding.agent = {&g_agentConn, &g_agentSession};
  binding.voice = &g_voiceUplink;
  binding.resolvePrompt = [hubResolver, agentResolver,
                           voiceResolver](const Page& page, PromptOutcome outcome) {
    if (page.source == "agent")
      agentResolver(page.id, outcome);
    else if (page.source == "voice")
      voiceResolver(page.id, outcome);
    else
      hubResolver(page.id, outcome);
  };
#else
  binding.resolvePrompt = [hubResolver](const Page& page, PromptOutcome outcome) {
    hubResolver(page.id, outcome);
  };
#endif
  g_runtime.bind(binding);
#if defined(TAMA_ENABLE_BUDDY)
  g_lineSink.bind(g_agentSession);
#endif

  g_runtime.begin();
  g_runtime.nav().add(screens::wifi());
  g_runtime.nav().setIr(&g_ir, &g_irStore);

#if defined(TAMA_ENABLE_BUDDY)
  if (const char* scenario = std::getenv("TAMA_BUDDY_STATE")) {
    feedBuddy(scenario);
    g_runtime.nav().start("buddy");
    const std::string sc = scenario;
    if (sc == "approve") {
      g_runtime.nav().dispatch(Intent::Select);
    } else if (sc == "deny") {
      g_runtime.nav().dispatch(Intent::Next);
      g_runtime.nav().dispatch(Intent::Select);
    }
  }
#endif

  if (const char* start = std::getenv("TAMA_START")) {
    g_runtime.nav().start(start);
  }

  g_capture.init();
}

void loop() {
  M5.update();
  pollKeys();
  const uint32_t now = static_cast<uint32_t>(m5gfx::millis());
#if defined(TAMA_ENABLE_BUDDY)
  g_lineSink.tick(now);
#endif
  g_capture.beforeFrame(now);
  g_runtime.loop(now);
  g_capture.afterFrame();
  M5.delay(8);
}
