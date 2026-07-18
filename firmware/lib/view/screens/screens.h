#pragma once

#include "navigator.h"
#include "prompt_overlay.h"
#include "registry.h"
#include "screen.h"

namespace tama::screens {

AppScreen& boot();
AppScreen& home();
AppScreen& menu();
AppScreen& metrics();
AppScreen& settings();
AppScreen& mascots();
AppScreen& play();
AppScreen& apps();
AppScreen& nook();
AppScreen& bluetooth();
AppScreen& wifi();
AppScreen& buddy();

void addSettingsScreens(Navigator& nav);

void registerAll(Navigator& nav);

void install(Navigator& nav, CharacterRegistry& characters, PromptOverlay& prompt);

}  // namespace tama::screens
