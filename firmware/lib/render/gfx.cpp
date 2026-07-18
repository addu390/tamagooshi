#include "gfx.h"

namespace tama {

#if defined(SDL_h_)
// Turn the desktop-sim window the same way the built-in L/R shortcut does, so a landscape
// panel rotation appears upright instead of sideways. Pushing a synthetic key event lets the
// SDL event thread own the actual window resize (required on macOS); doing it from the render
// thread is unsafe.
void simSyncWindowRotation(int panelRotation) {
  static int frame = 0;
  const int want = (panelRotation & 1) ? 3 : 0;
  while (frame != want) {
    SDL_Event ev{};
    ev.type = SDL_KEYDOWN;
    ev.key.type = SDL_KEYDOWN;
    ev.key.state = SDL_PRESSED;
    ev.key.keysym.mod = KMOD_NONE;
    if (((want - frame) & 3) <= ((frame - want) & 3)) {
      ev.key.keysym.sym = SDLK_r;
      frame = (frame + 1) & 3;
    } else {
      ev.key.keysym.sym = SDLK_l;
      frame = (frame - 1) & 3;
    }
    SDL_PushEvent(&ev);
  }
}
#endif

Gfx::Gfx() : canvas_(&M5.Display) {}

void Gfx::begin() {
  if (ready_) return;
  apply();
}

void Gfx::setRotation(int rotation) {
  if (ready_ && rotation == rot_) return;
  rot_ = rotation;
  apply();
}

void Gfx::apply() {
  M5.Display.setRotation(rot_);
#if defined(SDL_h_)
  simSyncWindowRotation(rot_);
#endif
  w_ = M5.Display.width();
  h_ = M5.Display.height();
  if (spriteCreated_) canvas_.deleteSprite();
  canvas_.setColorDepth(16);
  canvas_.createSprite(w_, h_);
  spriteCreated_ = true;
  ready_ = true;
}

void Gfx::clear(uint16_t color) {
  begin();
  canvas_.fillScreen(color);
}

void Gfx::push() { canvas_.pushSprite(0, 0); }

void Gfx::str(const char* s, int x, int y, uint16_t color, const lgfx::IFont* font,
              textdatum_t datum) {
  canvas_.setFont(font ? font : typeface::body());
  canvas_.setTextDatum(datum);
  canvas_.setTextColor(color);
  canvas_.drawString(s, x, y);
}

int Gfx::textWidth(const char* s, const lgfx::IFont* font) {
  canvas_.setFont(font);
  return canvas_.textWidth(s);
}

}  // namespace tama
