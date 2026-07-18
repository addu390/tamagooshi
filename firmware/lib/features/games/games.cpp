#include "games.h"

#include "brand.gen.h"

namespace tama::games {

namespace {

constexpr GameInfo kGames[] = {
#if TAMA_GAME_RUNNER
    {"runner", "DASH", "game.runner", false, false, false, nullptr},
#endif
#if TAMA_GAME_FLAPPY
    {"flappy", "FLAP", "game.flappy", false, false, false, nullptr},
#endif
#if TAMA_GAME_DELIVERY
    {"delivery", "DELIVERY", "game.delivery", false, false, false, nullptr},
#endif
#if TAMA_GAME_SCREAM
    {"scream", "SCREAM", "game.scream", false, false, true, nullptr},
#endif
#if TAMA_GAME_GALAXY
    {"galaxy", "GALAXY", "game.galaxy", false, true, false, nullptr},
#endif
#if TAMA_GAME_TILT
    {"tilt", "TILT MAZE", nullptr, false, true, false, "soon"},
#endif
};

}  // namespace

const GameInfo* list() { return kGames; }

int count() { return sizeof(kGames) / sizeof(kGames[0]); }

void registerAll(Navigator& nav) {
#if TAMA_GAME_RUNNER
  nav.add(runner());
#endif
#if TAMA_GAME_FLAPPY
  nav.add(flappy());
#endif
#if TAMA_GAME_DELIVERY
  nav.add(delivery());
#endif
#if TAMA_GAME_SCREAM
  nav.add(scream());
#endif
#if TAMA_GAME_GALAXY
  nav.add(galaxy());
#endif
}

}  // namespace tama::games
