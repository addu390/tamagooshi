#include "characters.h"

#include "mascots.gen.h"

namespace tama::characters {

void registerBuiltins(CharacterRegistry& registry) { registerGenerated(registry); }

}  // namespace tama::characters
