#include "pipeline.h"

#include "fields.h"
#include "types.h"

namespace tama {

namespace {

constexpr const char* kSource = "muse";
constexpr const char* kSaySource = "muse.say";
constexpr uint32_t kAlertReplyMarginMs = 5000;

const char* outcomeName(PromptOutcome outcome) {
  return outcome == PromptOutcome::Snooze ? "snooze" : "ack";
}

}  // namespace

MusePipeline::MusePipeline(ITransport& transport, DeviceState& state, MessageRouter& router)
    : transport_(transport), state_(state), router_(router) {}

void MusePipeline::onConnected(bool connected) {
  if (!connected) alert_.reset();
}

void MusePipeline::onInbound(const std::string& id, const std::string& payload) {
  muse::Invoke parsed;
  if (!muse::decodeInvoke(payload, parsed)) return reply(id, muse::encodeFailure("malformed invoke"));
  invoke(id, parsed);
}

void MusePipeline::invoke(const std::string& id, const muse::Invoke& invoke) {
  const muse::Command* command = muse::findCommand(invoke.command);
  if (!command) return reply(id, muse::encodeFailure("unknown command"));
  const std::string missing = muse::missingParam(command->spec, invoke.params);
  if (!missing.empty()) return reply(id, muse::encodeFailure("missing " + missing));
  run(id, command->verb, invoke.params);
}

void MusePipeline::run(const std::string& id, muse::Verb verb, const std::string& params) {
  switch (verb) {
    case muse::Verb::Describe:
      return reply(id, muse::encodeSuccess(muse::encodeDescription(state_)));
    case muse::Verb::Publish:
      route(mtype::kMetricUpsert, params);
      break;
    case muse::Verb::Clear: {
      const std::string key = muse::param(params, fields::kKey);
      if (key.empty())
        state_.clearMetrics();
      else
        state_.removeMetric(key);
      break;
    }
    case muse::Verb::Mood:
      if (moodFromString(muse::param(params, fields::kState)) == Mood::Unknown)
        return reply(id, muse::encodeFailure("unknown mood"));
      route(mtype::kMoodSet, params);
      break;
    case muse::Verb::Alert:
      return alert(id, params);
    case muse::Verb::Say:
      say(params);
      break;
    case muse::Verb::Feed:
      state_.pet.act(PetAction::Feed, now_);
      break;
    case muse::Verb::Play:
      state_.pet.act(PetAction::Play, now_);
      break;
    case muse::Verb::Love:
      state_.pet.act(PetAction::Love, now_);
      break;
  }
  reply(id, muse::encodeSuccess());
}

void MusePipeline::route(const char* type, const std::string& body) {
  Envelope env;
  env.v = fields::kProtocolVersion;
  env.type = type;
  env.src = kSource;
  env.body = body;
  router_.dispatch(env);
}

void MusePipeline::alert(const std::string& id, const std::string& params) {
  if (alert_) {
    state_.clearPrompt(alert_->page);
    settle("superseded");
  }
  const std::string page = std::string(kSource) + "." + id;
  alert_ = PendingAlert{id, page, now_};
  route(mtype::kPageRaise, muse::encodeAlertPage(params, page, kSource));
}

void MusePipeline::say(const std::string& params) {
  Page page;
  page.id = kSaySource;
  page.source = kSaySource;
  page.severity = Severity::Info;
  page.body = muse::param(params, "text");
  page.requires_ack = false;
  page.actions[0] = {"OK", PromptOutcome::Ack};
  page.actionCount = 1;
  state_.raisePrompt(page);
}

void MusePipeline::tick(uint32_t nowMs) {
  now_ = nowMs;
  if (!alert_ || nowMs - alert_->since < muse::kAlertTimeoutMs - kAlertReplyMarginMs) return;
  state_.clearPrompt(alert_->page);
  settle("timeout");
}

bool MusePipeline::resolve(const Page& page, PromptOutcome outcome) {
  if (page.source != kSource && page.source != kSaySource) return false;
  state_.clearPrompt(page.id);
  if (alert_ && alert_->page == page.id) settle(outcomeName(outcome));
  return true;
}

void MusePipeline::settle(const char* outcome) {
  const std::string request = alert_->request;
  alert_.reset();
  reply(request, muse::encodeSuccess(muse::encodeOutcome(outcome)));
}

void MusePipeline::reply(const std::string& id, const std::string& result) {
  transport_.publish(id, result);
}

}  // namespace tama
