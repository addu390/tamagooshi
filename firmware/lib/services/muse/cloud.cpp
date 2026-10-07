#include "cloud.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp32-hal-log.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <utility>

namespace tama {

namespace {

constexpr const char* kTag = "cloud";
constexpr const char* kApi = "https://api.muse.ai";
constexpr const char* kNoiseHost = "hatch.metaaivm.com";
constexpr int kTimeoutMs = 15000;
constexpr int kAttempts = 4;
constexpr uint32_t kFirstBackoffMs = 500;
constexpr size_t kMaxBody = 32 * 1024;
constexpr int kBufferBytes = 4096;

esp_err_t collect(esp_http_client_event_t* event) {
  if (event->event_id != HTTP_EVENT_ON_DATA) return ESP_OK;
  auto* out = static_cast<std::string*>(event->user_data);
  if (out->size() + event->data_len > kMaxBody) return ESP_FAIL;
  out->append(static_cast<const char*>(event->data), event->data_len);
  return ESP_OK;
}

bool retryable(int status) {
  return status <= 0 || status == 408 || status == 429 || (status >= 500 && status < 600);
}

std::string refreshAuth(const std::string& token) {
  const size_t colon = token.rfind(':');
  return "Bearer hatch_refresh:" + (colon == std::string::npos ? token : token.substr(colon + 1));
}

}  // namespace

MuseCloud::MuseCloud(IAccountRepository& accounts, std::string node, std::string sdkToken)
    : accounts_(accounts), node_(std::move(node)), sdkToken_(std::move(sdkToken)) {}

muse::Resolution MuseCloud::resolve(muse::Target& target) {
  auto account = accounts_.load();
  if (!account || !account->valid()) return muse::Resolution::Revoked;

  if (!reported_) {
    reported_ = true;
    refresh(*account);
  }

  muse::Vm vm;
  Reply reply = fetch(*account, vm);
  if (reply == Reply::Auth) {
    const Reply renewed = refresh(*account);
    if (renewed == Reply::Auth) return muse::Resolution::Revoked;
    if (renewed == Reply::Failed) return muse::Resolution::Failed;
    reply = fetch(*account, vm);
  }
  if (reply != Reply::Ok) return muse::Resolution::Failed;
  ESP_LOGI(kTag, "resolved vm %s", vm.id.c_str());

  target.host = account->noise.empty() ? kNoiseHost : account->noise;
  target.vm = vm.id;
  target.token = vm.token;
  return muse::Resolution::Ok;
}

MuseCloud::Reply MuseCloud::fetch(const Account& account, muse::Vm& vm) {
  std::string body;
  const Reply reply = call(account, "/fetch_vms", "Bearer " + account.access, nullptr, body);
  if (reply != Reply::Ok) return reply;
  return muse::decodeVm(body, vm) ? Reply::Ok : Reply::Failed;
}

MuseCloud::Reply MuseCloud::refresh(Account& account) {
  const std::string request = muse::encodeTokenRequest(node_, sdkToken_);
  std::string body;
  const Reply reply =
      call(account, "/device_token/refresh", refreshAuth(account.refresh), &request, body);
  if (reply != Reply::Ok) return reply;
  Account renewed = account;
  if (!muse::decodeTokens(body, renewed) || !accounts_.save(renewed)) return Reply::Failed;
  account = std::move(renewed);
  return Reply::Ok;
}

MuseCloud::Reply MuseCloud::call(const Account& account, const char* path,
                                 const std::string& auth, const std::string* body,
                                 std::string& out) {
  const std::string url = (account.api.empty() ? kApi : account.api) + path;
  uint32_t backoff = kFirstBackoffMs;
  for (int attempt = 1;; ++attempt) {
    out.clear();
    esp_http_client_config_t cfg = {};
    cfg.url = url.c_str();
    cfg.method = body ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    cfg.timeout_ms = kTimeoutMs;
    cfg.event_handler = collect;
    cfg.user_data = &out;
    cfg.crt_bundle_attach = esp_crt_bundle_attach;
    cfg.disable_auto_redirect = true;
    cfg.buffer_size = kBufferBytes;
    cfg.buffer_size_tx = kBufferBytes;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return Reply::Failed;
    esp_http_client_set_header(client, "Authorization", auth.c_str());
    esp_http_client_set_header(client, "X-API-Version", "1.0.0");
    if (body) {
      esp_http_client_set_header(client, "Content-Type", "application/json");
      esp_http_client_set_post_field(client, body->c_str(), static_cast<int>(body->size()));
    }
    const esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (status < 200 || status >= 300) {
      ESP_LOGW(kTag, "%s -> %d (%s)", path, status, esp_err_to_name(err));
    }
    if (status == 401 || status == 403) return Reply::Auth;
    if (err != ESP_OK) status = 0;
    if (status >= 200 && status < 300) return Reply::Ok;
    if (!retryable(status) || attempt == kAttempts) return Reply::Failed;
    vTaskDelay(pdMS_TO_TICKS(backoff));
    backoff *= 2;
  }
}

}  // namespace tama

#endif
