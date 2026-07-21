#pragma once

#include <functional>
#include <vector>

#include "context.h"
#include "gfx.h"
#include "intent.h"
#include "transport.h"
#include "model.h"
#include "mascots/registry.h"
#include "screen.h"

namespace tama {

class PromptOverlay;

class Navigator {
 public:
  Navigator(DeviceState& state, PetState& pet, const DeviceCapabilities& caps,
            CharacterRegistry& characters);

  void add(AppScreen& screen);
  void setPrompt(PromptOverlay& overlay);
  void setLink(ILink& link);
  void setWifi(IWifiControl* wifi);
  void setMic(IMicSource& mic);
  void setSensor(ISensorSource& sensor);
  void setButtons(IButtonSource& buttons);
  void setVoice(IVoiceUplink* voice);
  void setExpression(IExpressionSink& expression);
  void setIr(IIrTransceiver* ir, IIrStore* store);
  void setGamepad(IGamepadLink* gamepad);
  void setResolver(PromptResolver resolver);

  void start(const char* firstId);
  void dispatch(Intent intent);
  void raisePrompt(const Page& page);
  void clearPrompt(const std::string& id);
  void tick(uint32_t nowMs);
  void render(Gfx& g);

  void markDirty();
  bool consumeDirty();
  ExpressionState expressionState() const;

 private:
  ShellContext ctx();
  int effectiveRotation() const;
  AppScreen* find(const char* id) const;
  AppScreen* top() const;
  void go(const char* id);
  void replace(const char* id);
  void back();
  void home();
  void apply(const Transition& t);

  DeviceState& state_;
  PetState& pet_;
  const DeviceCapabilities& caps_;
  CharacterRegistry& characters_;
  PromptResolver resolver_;
  ILink* link_ = nullptr;
  IWifiControl* wifi_ = nullptr;
  IMicSource* mic_ = nullptr;
  ISensorSource* sensor_ = nullptr;
  IButtonSource* buttons_ = nullptr;
  IVoiceUplink* voice_ = nullptr;
  IExpressionSink* expression_ = nullptr;
  IIrTransceiver* ir_ = nullptr;
  IIrStore* irStore_ = nullptr;
  IGamepadLink* gamepad_ = nullptr;
  std::vector<AppScreen*> screens_;
  std::vector<AppScreen*> stack_;
  PromptOverlay* prompt_ = nullptr;
  bool promptActive_ = false;
  std::string activePageId_;
  bool dirty_ = true;
};

}  // namespace tama
