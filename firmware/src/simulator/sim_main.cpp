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

#include "capture.h"
#include "hid.h"
#include "hal/m5_buttons.h"
#include "hal/profile.h"
#include "hub/pipeline.h"
#include "input.h"
#include "json.h"
#include "link.h"
#include "screens.h"
#include "transport.h"
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
  std::string deviceName() const override { return "gooshi-a1b2"; }
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
  void join(const WifiCredentials& creds) override {
    if (std::find(nets_.begin(), nets_.end(), creds.ssid) == nets_.end())
      nets_.push_back(creds.ssid);
    select(creds.ssid);
  }
  std::vector<std::string> scan() override { return {"Home-5G", "Cafe-Guest", "Office"}; }
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

class SimAgent : public IAgent {
 public:
  AgentState state() const override { return state_; }
  std::string name() const override { return state_ == AgentState::Online ? "Muse" : ""; }
  std::string beacon() const override { return "homelink-51a0c3"; }
  void pair() override {
    state_ = AgentState::Pairing;
    since_ = m5gfx::millis();
  }
  void cancel() override { state_ = AgentState::Unpaired; }
  void unpair() override { state_ = AgentState::Unpaired; }

  void configure(AgentState state) { state_ = state; }

  void loop(uint32_t nowMs) {
    if (state_ == AgentState::Pairing && nowMs - since_ > kPairMs) state_ = AgentState::Online;
  }

 private:
  static constexpr uint32_t kPairMs = 3000;
  AgentState state_ = AgentState::Unpaired;
  uint32_t since_ = 0;
};

class SimChat : public IVoiceUplink {
 public:
  SimChat(const IAgent& agent, VoiceChat& voice) : agent_(agent), voice_(voice) {}

  bool ready() const override { return agent_.state() == AgentState::Online; }
  void beginRecording() override { sentAt_ = 0; }
  void feed(const int16_t*, size_t) override {}
  void finish(uint32_t elapsedMs) override {
    if (elapsedMs < kMinMs) return;
    sentAt_ = m5gfx::millis();
  }
  void cancel() override { sentAt_ = 0; }
  bool sending() const override { return sentAt_ != 0; }
  uint32_t limitMs() const override { return 15000; }

  void loop(uint32_t nowMs) {
    if (!sentAt_) return;
    const uint32_t t = nowMs - sentAt_;
    if (t < kThinkMs) return;
    if (voice_.phase == VoicePhase::Sending) {
      voice_.phase = VoicePhase::Thinking;
      voice_.transcript = "how is the build doing?";
    }
    if (t < kReplyMs) return;
    const std::string full = kReply;
    const size_t shown = std::min(full.size(), static_cast<size_t>((t - kReplyMs) / 25));
    voice_.phase = VoicePhase::Reply;
    voice_.reply = full.substr(0, shown);
    if (shown == full.size()) {
      voice_.reply_done = true;
      sentAt_ = 0;
    }
  }

 private:
  static constexpr uint32_t kMinMs = 300;
  static constexpr uint32_t kThinkMs = 600;
  static constexpr uint32_t kReplyMs = 1800;
  static constexpr const char* kReply =
      "All green. The last deploy finished four minutes ago and error rates are flat.";

  const IAgent& agent_;
  VoiceChat& voice_;
  uint32_t sentAt_ = 0;
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

class SimHid : public IHidLink {
 public:
  void activate() override { active_ = true; }
  void deactivate() override { active_ = false; }
  bool ready(HidCapability) const override { return active_; }
  void send(const GamepadFrame& frame) override { last_ = frame; }
  void tap(MediaKey) override {}
  void tap(KeyboardKey) override {}
  void pointer(int8_t, int8_t, uint8_t) override {}

 private:
  bool active_ = false;
  GamepadFrame last_;
};

class SimIrCodeRepository : public IIrCodeRepository {
 public:
  SimIrCodeRepository()
      : buttons_{{"BTN 1", SimIr::necLikeFrame()}, {"BTN 2", SimIr::necLikeFrame()}} {}

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

SimTransport g_sim;
NullExpression g_expression;
SimLink g_simLink;
SimWifi g_simWifi;
SimTelemetry g_telemetry;
M5Buttons g_buttons;
NullInputSource g_input;
SimSensor g_sensor;
SimMic g_mic;
SimIr g_ir;
SimIrCodeRepository g_irCodes;
SimHid g_hid;
SimConfig g_config;
NullMetricRepository g_metrics;
NullHidProfileRepository g_hidProfile;
NullClockRepository g_clock;
StaticBoardProfile g_board(board::capabilities());
IdFn g_idGen =
    ids::ulidGenerator([] { return nowMs(); }, [] { return static_cast<uint8_t>(rand()); });
ArduinoJsonCodec g_codec(g_idGen, [] { return nowMs(); }, kSimId);
SimSystemControl g_systemControl;
Runtime g_runtime(g_board.capabilities(), g_codec, g_expression, g_systemControl, g_buttons, g_input,
                  g_sensor, g_telemetry, g_mic, g_config, g_metrics, g_hidProfile, g_clock);
HubPipeline g_hubPipeline(g_sim, g_codec, g_runtime.router(), g_board, kSimId, "0.1.0-sim");
SimAgent g_agent;
SimChat g_chat(g_agent, g_runtime.state().voice);
sim::CaptureHarness g_capture(g_runtime);

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

  auto hubResolver = makeHubResolver(g_sim, g_codec, kSimId);

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

  if (const char* agent = std::getenv("TAMA_AGENT")) {
    const std::string s = agent;
    if (s == "online") g_agent.configure(AgentState::Online);
    if (s == "pairing") g_agent.configure(AgentState::Pairing);
  }

  ChannelBinding binding;
  binding.link = &g_simLink;
  binding.wifi = &g_simWifi;
  binding.voice = &g_chat;
  binding.assistant = &g_agent;
  binding.hub = {&g_sim, &g_hubPipeline};
  binding.resolvePrompt = [hubResolver](const Page& page, PromptOutcome outcome) {
    if (page.id == "hid.reboot") {
      if (outcome == PromptOutcome::Ack) g_systemControl.reboot();
      return;
    }
    hubResolver(page.id, outcome);
  };
  g_runtime.bind(binding);

  g_runtime.begin();
  g_runtime.nav().add(screens::wifi());
  g_runtime.nav().setIr(&g_ir, &g_irCodes);
  g_runtime.nav().setHid(&g_hid);

  if (const char* start = std::getenv("TAMA_START")) {
    g_runtime.nav().start(start);
  }

  g_capture.init();
}

void loop() {
  M5.update();
  pollKeys();
  const uint32_t now = static_cast<uint32_t>(m5gfx::millis());
  g_capture.beforeFrame(now);
  g_agent.loop(now);
  g_chat.loop(now);
  g_runtime.loop(now);
  g_capture.afterFrame();
  M5.delay(8);
}
