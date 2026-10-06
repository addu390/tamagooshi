#include "pet.h"

namespace tama {

namespace {
constexpr uint32_t kStepMs = 33;
}

void Pet::act(PetAction action, uint32_t nowMs) {
  switch (action) {
    case PetAction::Feed:
      emote(Expr::Celebrate, nowMs, 1400);
      spawnBerries(5);
      break;
    case PetAction::Play:
      emote(Expr::Celebrate, nowMs, 1600);
      spawnStars(6);
      break;
    case PetAction::Love:
      emote(Expr::Celebrate, nowMs, 1400);
      spawnHearts(6);
      break;
  }
}

bool Pet::tick(uint32_t nowMs, bool content) {
  if (nextCheer_ == 0) nextCheer_ = nowMs + 5000;
  if (nowMs >= nextCheer_) {
    nextCheer_ = nowMs + 4000 + (rng_.next() % 4000u);
    if (content && !reacting(nowMs)) {
      emote(Expr::Happy, nowMs, 1200);
      spawnHearts(2);
    }
  }
  if (particles_.empty() || nowMs < nextStep_) return false;
  nextStep_ = nowMs + kStepMs;
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
  return true;
}

void Pet::emote(Expr e, uint32_t nowMs, uint32_t ms) {
  reactExpr_ = e;
  reactUntil_ = nowMs + ms;
}

void Pet::spawnHearts(int n) {
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

void Pet::spawnBerries(int n) {
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

void Pet::spawnStars(int n) {
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
