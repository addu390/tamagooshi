#include <Arduino.h>
#include <M5Unified.h>
#include <esp_system.h>

#include <string>

#include "board.gen.h"
#include "brand.gen.h"
#include "hal/identity.h"
#include "hal/m5_buttons.h"
#include "hal/m5_expression.h"
#include "hal/m5_imu.h"
#if TAMA_APP_REMOTE && TAMA_BOARD_HAS_IR
#include "hal/m5_ir.h"
#endif
#include "hal/m5_mic.h"
#include "hal/m5_system.h"
#include "hal/m5_telemetry.h"
#include "hal/profile.h"
#if TAMA_NEEDS_HID
#include "nvs/hidprofile.h"
#endif
#if TAMA_APP_REMOTE && TAMA_BOARD_HAS_IR
#include "nvs/ircodes.h"
#endif
#include "nvs/clocks.h"
#include "nvs/metrics.h"
#include "nvs/radiostate.h"
#include "nvs/scope.h"
#include "partition/provisioning.h"
#include "hid.h"
#include "hub/pipeline.h"
#include "input.h"
#include "json.h"
#include "runtime.h"
#include "channels.h"
#include "ulid.h"

#include "ble/ble.h"
#if TAMA_NEEDS_HID
#include "ble/hid.h"
#endif
#include "transport/gatt.h"
#if defined(TAMA_ENABLE_WIFI)
#include "nvs/networks.h"
#include "wifi/softap.h"
#include "wifi/wifi.h"
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif
#if defined(TAMA_AGENT_MUSE)
#include "ble/setup.h"
#include "muse/agent.h"
#include "muse/chat.h"
#include "muse/cloud.h"
#include "muse/commands.h"
#include "muse/pipeline.h"
#include "nvs/accounts.h"
#include "transport/noise.h"
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
static PartitionConfigSource g_config;
static NvsMetricRepository g_metrics;
static NvsClockRepository g_clock;
#if TAMA_NEEDS_HID
static NvsHidProfileRepository g_hidProfile;
#else
static NullHidProfileRepository g_hidProfile;
#endif
#if TAMA_APP_REMOTE && TAMA_BOARD_HAS_IR
static M5IrTransceiver g_ir(board::kIrTxPin, board::kIrRxPin);
static NvsIrCodeRepository g_irCodes;
#endif
static StaticBoardProfile g_board(board::capabilities());

static Runtime g_runtime(g_board.capabilities(), g_codec, g_expression, g_system, g_buttons, g_input,
                         g_sensor, g_telemetry, g_mic, g_config, g_metrics, g_hidProfile, g_clock);

static NvsRadioStateRepository g_bleRadio(nvs::kBleRadio);
static BleBearer g_ble(TAMA_BRAND_ID, TAMA_FW_VERSION, g_deviceId, g_bleRadio);
static HubEndpoint g_hub(g_ble, TAMA_BRAND_ID, TAMA_FW_VERSION);

#if TAMA_NEEDS_HID
static HidEndpoint g_hid(TAMA_BRAND_ID, g_hidProfile);
#endif

#if defined(TAMA_ENABLE_WIFI)
static NvsNetworkRepository g_networks;
static NvsRadioStateRepository g_wifiRadio(nvs::kWifiRadio);
static SoftApProvisioner g_wifiProvisioner(TAMA_BRAND_ID "-setup");
static WifiBearer g_wifi(g_networks, g_wifiProvisioner, g_wifiRadio);
#endif

static HubPipeline g_hubPipeline(g_hub, g_codec, g_runtime.router(), g_board, g_deviceId,
                                 TAMA_FW_VERSION);

#if defined(TAMA_AGENT_MUSE)
static const muse::Device g_museDevice{identity::node(), "hatch-link:" + identity::mac(),
                                       identity::mac(), TAMA_FW_VERSION};
static NvsAccountRepository g_accounts;
static SetupEndpoint g_setup(g_ble, g_museDevice, identity::gadget(), TAMA_MUSE_SDK_TOKEN);
static MuseCloud g_cloud(g_accounts, g_museDevice.node, TAMA_MUSE_SDK_TOKEN);
static NoiseTransport g_noise(g_wifi,
                              {g_museDevice.node, TAMA_PRODUCT_NAME, TAMA_FW_VERSION, "",
                               muse::manifest()},
                              [](muse::Target& target) { return g_cloud.resolve(target); });
static MusePipeline g_musePipeline(g_noise, g_runtime.state(), g_runtime.router());
static MuseChat g_chat(g_noise, g_runtime.state().voice);
static MuseAgent g_muse(g_runtime.state(), g_accounts, g_wifi, g_setup, g_noise, g_chat, g_system,
                        g_clock);
#endif

static void configureChannels() {
  auto hubResolver = makeHubResolver(g_hub, g_codec, g_deviceId);

  ChannelBinding binding;
  binding.hub = {&g_hub, &g_hubPipeline};

#if defined(TAMA_ENABLE_WIFI)
  binding.wifi = &g_wifi;
#endif

  g_ble.add(g_hub);
#if TAMA_NEEDS_HID
  g_ble.add(g_hid);
#endif
#if defined(TAMA_AGENT_MUSE)
  g_ble.add(g_setup);
#endif
  binding.link = &g_ble;

#if defined(TAMA_AGENT_MUSE)
  binding.agent = {&g_noise, &g_musePipeline};
  g_chat.onInvoke([](const std::string& id, const muse::Invoke& invoke) {
    g_musePipeline.invoke(id, invoke);
  });
  binding.voice = &g_chat;
  binding.assistant = &g_muse;
#endif

  binding.resolvePrompt = [hubResolver](const Page& page, PromptOutcome outcome) {
    if (page.id == "hid.reboot") {
      if (outcome == PromptOutcome::Ack) g_system.reboot();
      return;
    }
#if defined(TAMA_AGENT_MUSE)
    if (g_musePipeline.resolve(page, outcome) || g_muse.resolve(page, outcome)) return;
#endif
    hubResolver(page.id, outcome);
  };

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

  configureChannels();

#if defined(TAMA_ENABLE_WIFI)
#if defined(TAMA_WIFI_SSID)
  WifiCredentials seed{TAMA_WIFI_SSID, TAMA_WIFI_PASSWORD};
  if (seed.valid() && g_networks.all().empty()) g_networks.remember(seed);
#endif
  g_wifi.begin();
#endif
  g_ble.begin();

  g_runtime.begin();
#if defined(TAMA_AGENT_MUSE)
  g_muse.begin();
#endif

#if TAMA_APP_REMOTE && TAMA_BOARD_HAS_IR
  g_ir.begin();
  g_runtime.nav().setIr(&g_ir, &g_irCodes);
#endif
#if TAMA_NEEDS_HID
  g_runtime.nav().setHid(&g_hid);
#endif
}

void loop() {
  M5.update();
  g_runtime.loop(millis());
#if defined(TAMA_ENABLE_WIFI)
  g_wifi.loop();
#endif
#if defined(TAMA_AGENT_MUSE)
  g_muse.loop(millis());
#endif
  delay(5);
}
