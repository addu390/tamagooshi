#include "brand.gen.h"
#if TAMA_APP_FLASHLIGHT

#include "apps.h"

namespace tama::apps {

namespace {

constexpr uint16_t kWhite = 0xFFFF;
constexpr uint16_t kInk = 0x8410;

class FlashlightScreen : public AppScreen {
 public:
  const char* id() const override { return "app.flashlight"; }
  OrientationPref orientation() const override { return OrientationPref::Portrait; }

  void onEnter(ShellContext& ctx) override {
    state_ = &ctx.state;
    prevBrightness_ = ctx.state.brightness;
    ctx.state.brightness = 255;
  }

  void onExit() override {
    if (state_) state_->brightness = prevBrightness_;
  }

  void render(Gfx& g, ShellContext&) override {
    g.clear(kWhite);
    g.str("TAP A TO EXIT", g.w() / 2, g.h() - 8, kInk, typeface::micro(),
          textdatum_t::bottom_center);
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Select) return Transition::back();
    return Transition::none();
  }

 private:
  DeviceState* state_ = nullptr;
  uint8_t prevBrightness_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(flashlight, FlashlightScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_FLASHLIGHT
