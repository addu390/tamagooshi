#pragma once

#include <cstdint>
#include <vector>

#include "mascot.h"
#include "rng.h"

namespace tama {

struct PetState {
  int energy = 72;
  int care = 55;
  int bond = 40;
  uint32_t lastDecayMs = 0;
};

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

class Care {
 public:
  void reset();

  bool decay(PetState& pet, uint32_t nowMs);
  bool spontaneous(PetState& pet, uint32_t nowMs);
  void stepParticles();

  void doAction(PetState& pet, PetAction action, uint32_t nowMs);
  void quickFeed(PetState& pet);

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
  uint32_t autoNext_ = 0;
  LcgRng rng_;
  std::vector<Particle> particles_;
};

}
