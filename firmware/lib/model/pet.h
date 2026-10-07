#pragma once

#include <cstdint>
#include <vector>

#include "mascot.h"
#include "rng.h"

namespace tama {

enum class PetAction { Feed, Play, Love };

enum class Fx { Heart, Berry, Star };

struct Particle {
  float x;
  float y;
  float vx;
  float vy;
  float g;
  int life;
  Fx kind;
};

class Pet {
 public:
  void act(PetAction action, uint32_t nowMs);
  bool tick(uint32_t nowMs, bool content);

  bool reacting(uint32_t nowMs) const { return nowMs < reactUntil_; }
  Expr reactExpr() const { return reactExpr_; }
  const std::vector<Particle>& particles() const { return particles_; }

 private:
  void emote(Expr e, uint32_t nowMs, uint32_t ms);
  void spawnHearts(int n);
  void spawnBerries(int n);
  void spawnStars(int n);

  Expr reactExpr_ = Expr::Happy;
  uint32_t reactUntil_ = 0;
  uint32_t nextCheer_ = 0;
  uint32_t nextStep_ = 0;
  LcgRng rng_;
  std::vector<Particle> particles_;
};

}  // namespace tama
