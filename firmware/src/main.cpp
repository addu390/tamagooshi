#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>

#include <string>

#include "board.gen.h"
#include "brand.gen.h"
#include "buddy/claude/commands.h"
#include "buddy/claude/session.h"
#include "buddy/controller.h"
#include "hal/identity.h"
#include "hal/m5_buttons.h"
#include "hal/m5_config.h"
#include "hal/m5_expression.h"
#include "hal/m5_imu.h"
#include "hal/m5_mic.h"
#include "hal/m5_system.h"
#include "hal/m5_telemetry.h"
#include "hal/profile.h"
#include "hub/pipeline.h"
#include "input.h"
#include "json.h"
#include "runtime.h"
#include "source.h"
#include "topics.h"
#include "ulid.h"

#if defined(TAMA_ENABLE_BLE)
#include "ble/ble.h"
#include "transport/nus.h"
#endif
#if defined(TAMA_PROTO_GATT)
#include "transport/gatt.h"
#endif
#if defined(TAMA_ENABLE_WIFI)
#include "secrets.h"
#include "wifi/nvs.h"
#include "wifi/softap.h"
#include "wifi/wifi.h"
#endif
#if defined(TAMA_PROTO_MQTT)
#include "transport/mqtt.h"
#endif

using namespace tama;

static uint64_t nowMs() { return static_cast<uint64_t>(millis()); }

static const std::string g_deviceId = identity::deviceId();

static IdFn g_idGen =
    ids::ulidGenerator([] { return nowMs(); }, [] { return static_cast<uint8_t>(esp_random()); });
static ArduinoJsonCodec g_codec(g_idGen, [] { return nowMs(); }, g_deviceId);

static M5Expression g_expression(board::kRedLedPin);
static M5Buttons g_buttons;
static M5SystemControl g_system;
static M5Telemetry g_telemetry;
static NullInputSource g_input;
static M5Imu g_sensor;
static M5Mic g_mic;
static M5Config g_config;
static StaticBoardProfile g_board(board::capabilities());

static Runtime g_runtime(g_board.capabilities(), g_codec, g_expression, g_system, g_buttons, g_input,
                         g_sensor, g_telemetry, g_mic, g_config);

#if defined(TAMA_ENABLE_BLE)
static BleBearer g_ble(TAMA_BRAND_ID, TAMA_FW_VERSION);
static NusEndpoint g_nus(g_ble);
static BuddyController g_buddyController(g_runtime.state());
static ClaudeCommands g_claudeCommands(g_ble, g_telemetry, g_runtime.state());
static ClaudeSession g_claudeSession(g_nus, g_buddyController, g_claudeCommands);
#else
static NullLink g_nullLink;
#endif

#if defined(TAMA_PROTO_GATT)
static HubEndpoint g_hub(g_ble, TAMA_BRAND_ID, TAMA_FW_VERSION);
#endif

#if defined(TAMA_ENABLE_WIFI)
static NvsCredentialStore g_wifiCreds;
static SoftApProvisioner g_wifiProvisioner(TAMA_BRAND_ID "-setup");
static WifiBearer g_wifi(g_wifiCreds, g_wifiProvisioner);
#endif
#if defined(TAMA_PROTO_MQTT)
static const std::string g_willTopic = topics::status(g_deviceId);
static const std::string g_willPayload = g_codec.encodeStatus(false, TAMA_FW_VERSION, "");
static MqttTransport g_mqtt(g_wifi, TAMA_MQTT_HOST, TAMA_MQTT_PORT, g_deviceId, g_willTopic,
                            g_willPayload);
#endif

#if defined(TAMA_PROTO_MQTT)
static ITransport& g_hubTransport = g_mqtt;
#elif defined(TAMA_PROTO_GATT)
static ITransport& g_hubTransport = g_hub;
#endif

static HubPipeline g_hubPipeline(g_hubTransport, g_codec, g_runtime.router(), g_board, g_deviceId,
                                 TAMA_FW_VERSION);

static void configureSources() {
  auto hubResolver = makeHubResolver(g_hubTransport, g_codec, g_deviceId);

  SourceBinding binding;
  binding.hub = {&g_hubTransport, &g_hubPipeline};

#if defined(TAMA_ENABLE_WIFI)
  binding.wifi = &g_wifi;
#endif

#if defined(TAMA_ENABLE_BLE)
  g_ble.add(g_nus);
#if defined(TAMA_PROTO_GATT)
  g_ble.add(g_hub);
#endif
  binding.link = &g_ble;
  binding.claude = {&g_nus, &g_claudeSession};

  auto claudeResolver = makeClaudeResolver(g_claudeSession);
  binding.resolvePrompt = [hubResolver, claudeResolver](const Page& page, PromptOutcome outcome) {
    if (page.source == "claude")
      claudeResolver(page.id, outcome);
    else
      hubResolver(page.id, outcome);
  };
#else
  binding.link = &g_nullLink;
  binding.resolvePrompt = [hubResolver](const Page& page, PromptOutcome outcome) {
    hubResolver(page.id, outcome);
  };
#endif

  g_runtime.bind(binding);
}

void setup() {
  auto cfg = M5.config();
  cfg.fallback_board = m5::board_t::TAMA_M5_FALLBACK_BOARD;
  M5.begin(cfg);
  M5.Display.setRotation(0);
  Serial.begin(115200);
  Serial.println("Tamagooshi firmware " TAMA_FW_VERSION);

  if (M5.Rtc.isEnabled()) M5.Rtc.setSystemTimeFromRtc();

  auto& sys = g_runtime.state().sysinfo;
  sys.fw = TAMA_FW_VERSION;
  sys.id = g_deviceId;
  sys.flash_kb = ESP.getFlashChipSize() / 1024;
  sys.heap_total_kb = ESP.getHeapSize() / 1024;

  configureSources();

#if defined(TAMA_ENABLE_WIFI)
  WifiCredentials seed{TAMA_WIFI_SSID, TAMA_WIFI_PASSWORD};
  if (seed.valid() && g_wifiCreds.all().empty()) g_wifiCreds.remember(seed);
  g_wifi.begin();
#endif
#if defined(TAMA_ENABLE_BLE)
  g_ble.begin();
#endif

  g_runtime.begin();
}

void loop() {
  M5.update();
  g_runtime.loop(millis());
#if defined(TAMA_ENABLE_WIFI)
  g_wifi.loop();
#endif
  delay(5);
}
