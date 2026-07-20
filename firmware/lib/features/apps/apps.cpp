#include "apps.h"

#include "apps.gen.h"

namespace tama::apps {

const FeatureInfo* list() { return kGenerated; }

int count() { return kGeneratedCount; }

void registerAll(Navigator& nav) { registerGenerated(nav); }

}  // namespace tama::apps
