#include "buddy.h"

namespace tama {

const char* buddyPhaseLabel(BuddyPhase phase) {
  switch (phase) {
    case BuddyPhase::Idle: return "all quiet";
    case BuddyPhase::Working: return "working";
    case BuddyPhase::Waiting: return "needs you";
    case BuddyPhase::Done: return "done";
    case BuddyPhase::Offline:
    default: return "offline";
  }
}

}  // namespace tama
