#include "pet.h"

#include <algorithm>

namespace tama {

namespace {
constexpr uint32_t kDecayMs = 6000;
}

void Care::reset() {
  particles_.clear();
  reactUntil_ = 0;
  autoNext_ = 0;
}

bool Care::decay(PetState& pet, uint32_t nowMs) {
  if (pet.lastDecayMs == 0) pet.lastDecayMs = nowMs;
  if (nowMs - pet.lastDecayMs <= kDecayMs) return false;

  pet.lastDecayMs = nowMs;
  pet.energy = std::max(0, pet.energy - 1);
  pet.care = std::max(0, pet.care - 1);
  pet.bond = std::max(0, pet.bond - 1);
  return true;
}

bool Care::spontaneous(PetState& pet, uint32_t nowMs) {
  if (autoNext_ == 0) autoNext_ = nowMs + 5000;
  if (nowMs < autoNext_) return false;

  autoNext_ = nowMs + 4000 + (rng_.next() % 4000u);
  if (pet.care > 45 && !reacting(nowMs)) {
    emote(Expr::Happy, nowMs, 1200);
    spawnHearts(2);
  }
  return true;
}

void Care::doAction(PetState& pet, PetAction action, uint32_t nowMs) {
  switch (action) {
    case PetAction::Feed:
      pet.energy = std::min(100, pet.energy + 16);
      pet.care = std::min(100, pet.care + 3);
      emote(Expr::Happy, nowMs, 1400);
      spawnBerries(5);
      break;
    case PetAction::Play:
      if (pet.energy < 10) {
        emote(Expr::Worried, nowMs, 1200);
        break;
      }
      pet.bond = std::min(100, pet.bond + 12);
      pet.energy = std::max(0, pet.energy - 8);
      pet.care = std::min(100, pet.care + 2);
      emote(Expr::Celebrate, nowMs, 1600);
      spawnStars(6);
      break;
    case PetAction::Love:
      pet.care = std::min(100, pet.care + 14);
      pet.bond = std::min(100, pet.bond + 6);
      emote(Expr::Happy, nowMs, 1400);
      spawnHearts(6);
      break;
  }
}

void Care::quickFeed(PetState& pet) {
  pet.energy = std::min(100, pet.energy + 6);
  pet.care = std::min(100, pet.care + 4);
}

void Care::stepParticles() {
  for (auto it = particles_.begin(); it != particles_.end();) {
    it->x += it->vx;
    it->y += it->vy;
    it->vy += it->g;
    if (--it->life <= 0) {
      it = particles_.erase(it);
    } else {
      ++it;
    }
  }
}

void Care::emote(Expr e, uint32_t nowMs, uint32_t ms) {
  reactExpr_ = e;
  reactUntil_ = nowMs + ms;
}

void Care::spawnHearts(int n) {
  for (int i = 0; i < n; ++i) {
    Particle p;
    p.x = static_cast<float>(static_cast<int>(rng_.next() % 44u) - 22);
    p.y = static_cast<float>(-6 + (static_cast<int>(rng_.next() % 14u) - 7));
    p.vx = 0.0f;
    p.vy = -0.7f - static_cast<float>(rng_.next() % 70u) / 100.0f;
    p.g = 0.0f;
    p.life = 24 + static_cast<int>(rng_.next() % 18u);
    p.kind = Fx::Heart;
    particles_.push_back(p);
  }
}

void Care::spawnBerries(int n) {
  for (int i = 0; i < n; ++i) {
    Particle p;
    p.x = static_cast<float>(static_cast<int>(rng_.next() % 24u) - 12);
    p.y = static_cast<float>(-28 - static_cast<int>(rng_.next() % 6u));
    p.vx = static_cast<float>(static_cast<int>(rng_.next() % 50u) - 25) / 100.0f;
    p.vy = 0.3f + static_cast<float>(rng_.next() % 40u) / 100.0f;
    p.g = 0.09f;
    p.life = 22 + static_cast<int>(rng_.next() % 12u);
    p.kind = Fx::Berry;
    particles_.push_back(p);
  }
}

void Care::spawnStars(int n) {
  for (int i = 0; i < n; ++i) {
    Particle p;
    p.x = static_cast<float>(static_cast<int>(rng_.next() % 20u) - 10);
    p.y = static_cast<float>(-8 + (static_cast<int>(rng_.next() % 12u) - 6));
    p.vx = static_cast<float>(static_cast<int>(rng_.next() % 320u) - 160) / 100.0f;
    p.vy = -1.7f - static_cast<float>(rng_.next() % 90u) / 100.0f;
    p.g = 0.13f;
    p.life = 22 + static_cast<int>(rng_.next() % 16u);
    p.kind = Fx::Star;
    particles_.push_back(p);
  }
}

}
