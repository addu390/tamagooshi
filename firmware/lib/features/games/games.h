#pragma once

#include "feature.h"
#include "navigator.h"
#include "screen.h"

namespace tama::games {

const FeatureInfo* list();
int count();
void registerAll(Navigator& nav);

}  // namespace tama::games
