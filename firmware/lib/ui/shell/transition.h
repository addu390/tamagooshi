#pragma once

namespace tama {

enum class Nav { None, Push, Replace, Back, Home, Redraw };

struct Transition {
  Nav nav = Nav::None;
  const char* target = nullptr;

  static Transition none() { return {}; }
  static Transition redraw() { return {Nav::Redraw, nullptr}; }
  static Transition push(const char* id) { return {Nav::Push, id}; }
  static Transition replace(const char* id) { return {Nav::Replace, id}; }
  static Transition back() { return {Nav::Back, nullptr}; }
  static Transition home() { return {Nav::Home, nullptr}; }
};

}  // namespace tama
