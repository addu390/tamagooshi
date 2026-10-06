#include "commands.h"

#include "fields.h"

namespace tama::muse {

namespace {

constexpr Param kPublish[] = {
    {fields::kKey, "string", "Stable id for the metric, e.g. \"build\"", true},
    {fields::kLabel, "string", "Short label shown on screen, e.g. \"CI\"", true},
    {fields::kValue, "string", "Value shown on screen, e.g. \"green\" or \"42%\"", true},
    {fields::kTrend, "string", "Optional arrow: \"up\", \"down\" or \"flat\"", false},
    {fields::kKind, "string", "\"star\" pins the metric to the home screen", false},
};

constexpr Param kClear[] = {
    {fields::kKey, "string", "Metric to remove; omit to remove every metric", false},
};

constexpr Param kMood[] = {
    {fields::kState, "string", "happy, neutral, sick, panic, celebrate or sleepy", true},
    {fields::kReason, "string", "Why, shown under the pet", false},
};

constexpr Param kAlert[] = {
    {fields::kTitle, "string", "Headline, a few words", true},
    {fields::kBody, "string", "Detail text", false},
    {fields::kSeverity, "string", "info, warning or critical (default warning)", false},
};

constexpr Param kSay[] = {
    {"text", "string", "What the pet should say", true},
};

template <size_t N>
constexpr size_t count(const Param (&)[N]) {
  return N;
}

constexpr Command kCommands[] = {
    {{"pet.describe", "Report the pet: its name, mood, battery and the metrics on its screen",
      nullptr, 0, 0},
     Verb::Describe},
    {{"metrics.publish",
      "Show or update a metric on the pet's screen. Its mood and alert rules react to metrics.",
      kPublish, count(kPublish), 0},
     Verb::Publish},
    {{"metrics.clear", "Remove one metric, or all of them", kClear, count(kClear), 0},
     Verb::Clear},
    {{"pet.set_mood", "Set the pet's mood until its metric rules change it", kMood, count(kMood),
      0},
     Verb::Mood},
    {{"pet.alert",
      "Page the user on the pet and wait for a button press. Replies with outcome ack, snooze "
      "or timeout.",
      kAlert, count(kAlert), kAlertTimeoutMs},
     Verb::Alert},
    {{"pet.say", "Show a short message on the pet's screen", kSay, count(kSay), 0}, Verb::Say},
    {{"pet.feed", "Feed the pet a snack; it munches happily on screen", nullptr, 0, 0},
     Verb::Feed},
    {{"pet.play", "Play with the pet; it bounces around celebrating", nullptr, 0, 0},
     Verb::Play},
    {{"pet.love", "Pet and cuddle the pet; it beams with hearts", nullptr, 0, 0}, Verb::Love},
};

constexpr size_t kCount = sizeof(kCommands) / sizeof(kCommands[0]);

}  // namespace

const Command* findCommand(const std::string& name) {
  for (const Command& command : kCommands) {
    if (name == command.spec.name) return &command;
  }
  return nullptr;
}

std::string manifest() {
  CommandSpec specs[kCount];
  for (size_t i = 0; i < kCount; ++i) specs[i] = kCommands[i].spec;
  return encodeManifest(specs, kCount);
}

}  // namespace tama::muse
