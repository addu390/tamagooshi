#include "agent.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp32-hal-log.h>
#include <esp_sntp.h>

#include <ctime>

namespace tama {

namespace {

constexpr const char* kTag = "muse";
constexpr const char* kPairPage = "muse.pair";
constexpr uint32_t kJoinTimeoutMs = 30000;
constexpr uint32_t kCloseDelayMs = 2000;
constexpr time_t kValidEpoch = 1700000000;

}  // namespace

MuseAgent::MuseAgent(DeviceState& state, IAccountRepository& accounts, IWifiControl& wifi,
                     SetupEndpoint& setup, NoiseTransport& link, MuseChat& chat,
                     ISystemControl& system, IClockRepository& clock)
    : state_(state),
      accounts_(accounts),
      wifi_(wifi),
      setup_(setup),
      link_(link),
      chat_(chat),
      system_(system),
      clock_(clock) {}

void MuseAgent::begin() {
  const auto account = accounts_.load();
  paired_ = account && account->valid();
  setup_.onConfirm([this] {
    ESP_LOGI(kTag, "app asks to confirm pairing");
    Page page;
    page.id = kPairPage;
    page.source = kPairPage;
    page.severity = Severity::Warning;
    page.title = "PAIR MUSE?";
    page.body = "Allow the Muse app to set up " + beacon();
    page.actions[0] = {"ALLOW", PromptOutcome::Allow};
    page.actions[1] = {"DENY", PromptOutcome::Deny};
    page.actionCount = 2;
    state_.raisePrompt(page);
  });
  setup_.onProvision([this](const muse::Provision& p) { provision(p); });
  setup_.onScan([this] { return wifi_.scan(); });
}

void MuseAgent::loop(uint32_t nowMs) {
  now_ = nowMs;
  setup_.loop();
  if (joining_) join(nowMs);
  if (closeAt_ && nowMs >= closeAt_) {
    closeAt_ = 0;
    setup_.close();
  }
  sync();
  connect();
  watch();
  chat_.loop(nowMs);

  const AgentState current = state();
  if (current != shown_) {
    shown_ = current;
    state_.dirty = true;
  }
}

bool MuseAgent::resolve(const Page& page, PromptOutcome outcome) {
  if (page.source != kPairPage) return false;
  state_.clearPrompt(page.id);
  setup_.confirm(outcome == PromptOutcome::Allow);
  return true;
}

AgentState MuseAgent::state() const {
  if (setup_.isOpen() && !closeAt_) {
    if (joining_) return AgentState::Joining;
    if (setup_.phase() == Pairing::Phase::Confirming) return AgentState::Confirming;
    return AgentState::Pairing;
  }
  if (!paired_) return AgentState::Unpaired;
  return link_.connected() ? AgentState::Online : AgentState::Connecting;
}

void MuseAgent::pair() {
  if (setup_.isOpen()) return;
  ESP_LOGI(kTag, "pairing window open as %s", setup_.name().c_str());
  setup_.open(paired_);
}

void MuseAgent::cancel() {
  joining_.reset();
  closeAt_ = 0;
  state_.clearPrompt(kPairPage);
  setup_.close();
}

void MuseAgent::unpair() {
  cancel();
  link_.stop();
  chat_.cancel();
  accounts_.clear();
  paired_ = false;
  linked_ = false;
}

void MuseAgent::provision(const muse::Provision& provision) {
  ESP_LOGI(kTag, "provisioned, joining %s", provision.ssid.c_str());
  joining_ = provision;
  joinedAt_ = now_;
  setup_.status("wifi_connecting");
  wifi_.join({provision.ssid, provision.password});
}

void MuseAgent::join(uint32_t nowMs) {
  if (wifi_.connected() && wifi_.peer() == joining_->ssid) {
    setup_.status("wifi_connected");
    if (!accounts_.save(joining_->account)) {
      setup_.status("error_storage");
      joining_.reset();
      return;
    }
    setup_.status("auth_ok");
    ESP_LOGI(kTag, "joined wifi, account saved");
    joining_.reset();
    link_.stop();
    paired_ = true;
    linked_ = false;
    closeAt_ = nowMs + kCloseDelayMs;
  } else if (nowMs - joinedAt_ > kJoinTimeoutMs) {
    setup_.status("wifi_failed");
    ESP_LOGW(kTag, "wifi join timed out");
    joining_.reset();
  }
}

void MuseAgent::sync() {
  if (timeSynced_ || !wifi_.connected()) return;
  if (!timeRequested_) {
    timeRequested_ = true;
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    esp_sntp_init();
  }
  const time_t now = time(nullptr);
  if (now < kValidEpoch) return;
  timeSynced_ = true;
  system_.setClock(now);
  clock_.save({now, state_.tz_offset_min});
}

void MuseAgent::connect() {
  if (!paired_ || linked_ || !timeSynced_ || !wifi_.connected()) return;
  ESP_LOGI(kTag, "starting link on %s", wifi_.peer().c_str());
  link_.start(wifi_.peer());
  linked_ = true;
}

void MuseAgent::watch() {
  if (!linked_ || link_.fault() == muse::Fault::None) return;
  ESP_LOGW(kTag, "link fault, clearing account");
  link_.stop();
  chat_.cancel();
  accounts_.clear();
  paired_ = false;
  linked_ = false;
}

}  // namespace tama

#endif
