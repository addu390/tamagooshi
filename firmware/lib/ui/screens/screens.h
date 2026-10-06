#pragma once

#include "navigator.h"
#include "prompt.h"
#include "mascots/registry.h"
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
AppScreen& bluetooth();
AppScreen& wifi();
AppScreen& agent();
AppScreen& persona();

void addSettingsScreens(Navigator& nav);

const char* agentStatus(AgentState state);

void registerAll(Navigator& nav);

void install(Navigator& nav, CharacterRegistry& characters, PromptOverlay& prompt);

}  // namespace tama::screens
