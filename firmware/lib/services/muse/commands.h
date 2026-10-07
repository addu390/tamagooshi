#pragma once

#include <cstdint>
#include <string>

#include "muse.h"

namespace tama::muse {

enum class Verb { Describe, Publish, Clear, Mood, Alert, Say, Feed, Play, Love };

struct Command {
  CommandSpec spec;
  Verb verb;
};

inline constexpr uint32_t kAlertTimeoutMs = 120000;

const Command* findCommand(const std::string& name);

std::string manifest();

}  // namespace tama::muse
