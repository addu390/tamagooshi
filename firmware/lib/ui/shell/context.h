#pragma once

#include <functional>
#include <string>

#include "mascots/character.h"
#include "expression.h"
#include "gamepad.h"
#include "input.h"
#include "ir.h"
#include "mascots/registry.h"
#include "mascot.h"
#include "model.h"
#include "transport.h"
#include "wifi/control.h"

namespace tama {

using PromptResolver = std::function<void(const Page& page, PromptOutcome outcome)>;

struct ShellContext {
  DeviceState& state;
  PetState& pet;
  const DeviceCapabilities& caps;
  CharacterRegistry& characters;
  Character* character;
  MascotState mascot;
  bool promptActive;
  PromptResolver resolvePrompt;
  ILink& link;
  IWifiControl* wifi;
  IMicSource& mic;
  ISensorSource& sensor;
  IButtonSource& buttons;
  IVoiceUplink* voice;
  IExpressionSink* expression;
  IIrTransceiver* ir;
  IIrStore* irStore;
  IGamepadLink* gamepad;
};

}  // namespace tama
