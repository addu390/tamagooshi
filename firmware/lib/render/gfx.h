#pragma once

#include <M5Unified.h>

#include "theme.h"
#include "typeface.h"

namespace tama {

class Gfx {
 public:
  Gfx();

  void begin();
  void setRotation(int rotation);
  int rotation() const { return rot_; }
  int w() const { return w_; }
  int h() const { return h_; }
  M5Canvas& c() { return canvas_; }

  void clear(uint16_t color = theme::kBg);
  void push();
  void str(const char* s, int x, int y, uint16_t color, const lgfx::IFont* font = nullptr,
           textdatum_t datum = textdatum_t::top_left);
  int textWidth(const char* s, const lgfx::IFont* font);

 private:
  void apply();

  M5Canvas canvas_;
  bool ready_ = false;
  bool spriteCreated_ = false;
  int rot_ = 0;
  int w_ = 0;
  int h_ = 0;
};

}  // namespace tama
