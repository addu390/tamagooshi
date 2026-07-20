#include "games.h"

#include "games.gen.h"

namespace tama::games {

const FeatureInfo* list() { return kGenerated; }

int count() { return kGeneratedCount; }

void registerAll(Navigator& nav) { registerGenerated(nav); }

}  // namespace tama::games
