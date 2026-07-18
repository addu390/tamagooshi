#include <M5Unified.h>
#include <SDL.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "adapters.h"
#include "board.gen.h"
#include "buddy/claude/commands.h"
#include "buddy/claude/session.h"
#include "buddy/controller.h"
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
#include "source.h"
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
  std::string portal() const override { return "Claude-gooshi-a1b2"; }

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
    int n = 0;
    const Uint8* ks = SDL_GetKeyboardState(&n);
    const bool loud = ks && ks[SDL_SCANCODE_UP];
    level_ += ((loud ? 700 : 0) - level_) / 3;
    return level_;
  }

 private:
  bool active_ = false;
  int level_ = 0;
};

SimTransport g_sim;
SimTransport g_claudeConn;
MqttSimTransport g_mqtt;
TransportProxy g_transport;
NullExpression g_expression;
SimLink g_simLink;
SimWifi g_simWifi;
NullLineSink g_lineSink;
SimTelemetry g_telemetry;
M5Buttons g_buttons;
NullInputSource g_input;
SimSensor g_sensor;
SimMic g_mic;
SimConfig g_config;
StaticBoardProfile g_board(board::capabilities());
IdFn g_idGen =
    ids::ulidGenerator([] { return nowMs(); }, [] { return static_cast<uint8_t>(rand()); });
ArduinoJsonCodec g_codec(g_idGen, [] { return nowMs(); }, kSimId);
SimSystemControl g_systemControl;
Runtime g_runtime(g_board.capabilities(), g_codec, g_expression, g_systemControl, g_buttons, g_input,
                  g_sensor, g_telemetry, g_mic, g_config);
HubPipeline g_hubPipeline(g_transport, g_codec, g_runtime.router(), g_board, kSimId, "0.1.0-sim");
BuddyController g_buddyController(g_runtime.state());
ClaudeCommands g_claudeCommands(g_simLink, g_telemetry, g_runtime.state());
ClaudeSession g_claudeSession(g_lineSink, g_buddyController, g_claudeCommands);
sim::CaptureHarness g_capture(g_runtime, g_claudeSession);

void feedClaude(const char* scenario) {
  g_claudeSession.onInbound("", "{\"cmd\":\"owner\",\"name\":\"John\"}");

  const std::string s = scenario ? scenario : "working";
  if (s == "idle") {
    g_claudeSession.onInbound(
        "", "{\"total\":0,\"running\":0,\"waiting\":0,\"tokens\":184502,\"tokens_today\":31200}");
  } else if (s == "waiting" || s == "approve" || s == "deny") {
    g_claudeSession.onInbound(
        "",
        "{\"total\":3,\"running\":1,\"waiting\":1,\"msg\":\"approve: Bash\",\"entries\":[\"10:42 "
        "git push\"],\"tokens\":184502,\"tokens_today\":31200,\"prompt\":{\"id\":\"req_abc123\","
        "\"tool\":\"Bash\",\"hint\":\"rm -rf /tmp/foo\"}}");
  } else {
    g_claudeSession.onInbound(
        "",
        "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"editing files\",\"entries\":[\"10:42 git "
        "push\",\"10:41 yarn test\",\"10:39 reading file...\"],\"tokens\":184502,\"tokens_today\":"
        "31200}");
  }
}

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

  const char* claude = std::getenv("TAMA_CLAUDE");
  const char* broker = std::getenv("TAMA_BROKER");

  g_transport.bind(broker && g_mqtt.tryConnect(broker, kSimId)
                       ? static_cast<ITransport*>(&g_mqtt)
                       : static_cast<ITransport*>(&g_sim));

  auto hubResolver = makeHubResolver(g_transport, g_codec, kSimId);
  auto claudeResolver = makeClaudeResolver(g_claudeSession);

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

  SourceBinding binding;
  binding.link = &g_simLink;
  binding.wifi = &g_simWifi;
  binding.hub = {&g_transport, &g_hubPipeline};
  binding.claude = {&g_claudeConn, &g_claudeSession};
  binding.resolvePrompt = [hubResolver, claudeResolver](const Page& page, PromptOutcome outcome) {
    if (page.source == "claude")
      claudeResolver(page.id, outcome);
    else
      hubResolver(page.id, outcome);
  };
  g_runtime.bind(binding);

  g_runtime.begin();
  g_runtime.nav().add(screens::wifi());

  if (claude) {
    feedClaude(claude);
    g_runtime.nav().start("buddy");
    const std::string sc = claude;
    if (sc == "approve") {
      g_runtime.nav().dispatch(Intent::Select);
    } else if (sc == "deny") {
      g_runtime.nav().dispatch(Intent::Next);
      g_runtime.nav().dispatch(Intent::Select);
    }
  }

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
  g_runtime.loop(now);
  g_capture.afterFrame();
  M5.delay(8);
}
