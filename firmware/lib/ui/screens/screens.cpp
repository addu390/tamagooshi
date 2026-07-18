#include "screens.h"

#include "apps/apps.h"
#include "brand.gen.h"
#include "mascots/characters.h"
#include "games/games.h"

namespace tama::screens {

void registerAll(Navigator& nav) {
  nav.add(boot());
  nav.add(home());
  nav.add(menu());
  nav.add(metrics());
  addSettingsScreens(nav);
  nav.add(mascots());
  nav.add(play());
  nav.add(apps());
  nav.add(nook());
  nav.add(bluetooth());
#if defined(TAMA_ENABLE_WIFI)
  nav.add(wifi());
#endif
#if defined(TAMA_ENABLE_BUDDY)
  nav.add(buddy());
  nav.add(ask());
#endif
}

void install(Navigator& nav, CharacterRegistry& characters, PromptOverlay& prompt) {
  characters::registerBuiltins(characters);
  nav.setPrompt(prompt);
  registerAll(nav);
  games::registerAll(nav);
  apps::registerAll(nav);
}

}  // namespace tama::screens
