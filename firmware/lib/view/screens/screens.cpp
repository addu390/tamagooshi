#include "screens.h"

#include "apps.h"
#include "characters.h"
#include "games.h"

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
  nav.add(buddy());
}

void install(Navigator& nav, CharacterRegistry& characters, PromptOverlay& prompt) {
  characters::registerBuiltins(characters);
  nav.setPrompt(prompt);
  registerAll(nav);
  games::registerAll(nav);
  apps::registerAll(nav);
}

}  // namespace tama::screens
