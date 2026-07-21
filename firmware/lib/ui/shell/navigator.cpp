#include "navigator.h"

#include <cstring>
#include <utility>

#include "prompt_overlay.h"

namespace tama {

namespace {
NullLink g_nullLink;
}

Navigator::Navigator(DeviceState& state, PetState& pet, const DeviceCapabilities& caps,
                     CharacterRegistry& characters)
    : state_(state), pet_(pet), caps_(caps), characters_(characters), link_(&g_nullLink) {}

void Navigator::add(AppScreen& screen) { screens_.push_back(&screen); }

void Navigator::setPrompt(PromptOverlay& overlay) { prompt_ = &overlay; }

void Navigator::setLink(ILink& link) { link_ = &link; }

void Navigator::setWifi(IWifiControl* wifi) { wifi_ = wifi; }

void Navigator::setMic(IMicSource& mic) { mic_ = &mic; }

void Navigator::setSensor(ISensorSource& sensor) { sensor_ = &sensor; }

void Navigator::setButtons(IButtonSource& buttons) { buttons_ = &buttons; }

void Navigator::setVoice(IVoiceUplink* voice) { voice_ = voice; }

void Navigator::setExpression(IExpressionSink& expression) { expression_ = &expression; }

void Navigator::setIr(IIrTransceiver* ir, IIrStore* store) {
  ir_ = ir;
  irStore_ = store;
}

void Navigator::setGamepad(IGamepadLink* gamepad) { gamepad_ = gamepad; }

void Navigator::setResolver(PromptResolver resolver) { resolver_ = std::move(resolver); }

AppScreen* Navigator::find(const char* id) const {
  for (auto* s : screens_) {
    if (std::strcmp(s->id(), id) == 0) return s;
  }
  return nullptr;
}

AppScreen* Navigator::top() const { return stack_.empty() ? nullptr : stack_.back(); }

ShellContext Navigator::ctx() {
  return ShellContext{state_,
                      pet_,
                      caps_,
                      characters_,
                      characters_.getOrDefault(state_.character_id),
                      deriveMascot(state_, promptActive_),
                      promptActive_,
                      resolver_,
                      *link_,
                      wifi_,
                      *mic_,
                      *sensor_,
                      *buttons_,
                      voice_,
                      expression_,
                      ir_,
                      irStore_,
                      gamepad_};
}

void Navigator::start(const char* firstId) {
  stack_.clear();
  if (auto* s = find(firstId)) {
    stack_.push_back(s);
    ShellContext c = ctx();
    s->onEnter(c);
  }
  markDirty();
}

void Navigator::go(const char* id) {
  auto* s = find(id);
  if (!s) return;
  if (auto* t = top()) t->onExit();
  stack_.push_back(s);
  ShellContext c = ctx();
  s->onEnter(c);
  markDirty();
}

void Navigator::replace(const char* id) {
  auto* s = find(id);
  if (!s) return;
  if (auto* t = top()) {
    t->onExit();
    stack_.pop_back();
  }
  stack_.push_back(s);
  ShellContext c = ctx();
  s->onEnter(c);
  markDirty();
}

void Navigator::back() {
  if (stack_.size() > 1) {
    top()->onExit();
    stack_.pop_back();
    ShellContext c = ctx();
    top()->onEnter(c);
  }
  markDirty();
}

void Navigator::home() {
  while (stack_.size() > 1) {
    top()->onExit();
    stack_.pop_back();
  }
  markDirty();
}

void Navigator::apply(const Transition& t) {
  switch (t.nav) {
    case Nav::Push: go(t.target); break;
    case Nav::Replace: replace(t.target); break;
    case Nav::Back: back(); break;
    case Nav::Home: home(); break;
    case Nav::Redraw: markDirty(); break;
    case Nav::None: default: break;
  }
}

void Navigator::dispatch(Intent intent) {
  if (promptActive_ && prompt_) {
    ShellContext c = ctx();
    Transition t = prompt_->handleInput(intent, c);
    if (t.nav == Nav::Back || t.nav == Nav::Home) promptActive_ = false;
    if (t.nav == Nav::Home) home();
    markDirty();
    return;
  }
  if (intent == Intent::Home) {
    home();
    return;
  }
  if (intent == Intent::Back) {
    back();
    return;
  }
  if (auto* t = top()) {
    ShellContext c = ctx();
    apply(t->handleInput(intent, c));
  }
}

void Navigator::raisePrompt(const Page& page) {
  if (!prompt_) return;
  prompt_->set(page);
  promptActive_ = true;
  activePageId_ = page.id;
  markDirty();
}

void Navigator::clearPrompt(const std::string& id) {
  if (!promptActive_) return;
  if (!id.empty() && id != activePageId_) return;
  promptActive_ = false;
  activePageId_.clear();
  markDirty();
}

void Navigator::tick(uint32_t nowMs) {
  if (promptActive_ && prompt_) {
    ShellContext c = ctx();
    if (prompt_->tick(c, nowMs).nav == Nav::Redraw) markDirty();
    return;
  }
  if (auto* t = top()) {
    ShellContext c = ctx();
    apply(t->tick(c, nowMs));
  }
}

void Navigator::render(Gfx& g) {
  g.setRotation(effectiveRotation());
  g.clear();
  ShellContext c = ctx();
  if (promptActive_ && prompt_) {
    prompt_->render(g, c);
  } else if (auto* t = top()) {
    t->render(g, c);
  }
  g.push();
}

int Navigator::effectiveRotation() const {
  if (!promptActive_) {
    if (auto* t = top()) {
      switch (t->orientation()) {
        case OrientationPref::Portrait: return rotationFor(Orientation::Portrait);
        case OrientationPref::Landscape: return rotationFor(Orientation::Landscape);
        case OrientationPref::PortraitFlipped: return rotationFor(Orientation::Portrait) + 2;
        case OrientationPref::Inherit: break;
      }
    }
  }
  return rotationFor(state_.orientation);
}

void Navigator::markDirty() { dirty_ = true; }

bool Navigator::consumeDirty() {
  const bool was = dirty_;
  dirty_ = false;
  return was;
}

ExpressionState Navigator::expressionState() const {
  ExpressionState s;
  s.muted = state_.muted;
  if (promptActive_ && prompt_) {
    s.alertActive = true;
    s.buzzerActive = prompt_->buzzes() && state_.buzzer_enabled;
  }
  return s;
}

}  // namespace tama
