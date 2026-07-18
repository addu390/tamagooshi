#include "registry.h"

#include <cstring>

namespace tama {

void CharacterRegistry::add(Character& character) { chars_.push_back(&character); }

Character* CharacterRegistry::first() const { return chars_.empty() ? nullptr : chars_.front(); }

Character* CharacterRegistry::get(const std::string& id) const {
  for (auto* c : chars_) {
    if (id == c->id()) return c;
  }
  return nullptr;
}

Character* CharacterRegistry::getOrDefault(const std::string& id) const {
  Character* c = get(id);
  return c ? c : first();
}

Character* CharacterRegistry::byIndex(int index) const {
  if (index < 0 || index >= count()) return nullptr;
  return chars_[index];
}

int CharacterRegistry::indexOf(const std::string& id) const {
  for (int i = 0; i < count(); ++i) {
    if (id == chars_[i]->id()) return i;
  }
  return 0;
}

bool CharacterRegistry::isFirstInCategory(int index) const {
  const char* c = chars_[index]->category();
  for (int j = 0; j < index; ++j) {
    if (std::strcmp(chars_[j]->category(), c) == 0) return false;
  }
  return true;
}

int CharacterRegistry::categoryCount() const {
  int n = 0;
  for (int i = 0; i < count(); ++i) {
    if (isFirstInCategory(i)) ++n;
  }
  return n;
}

const char* CharacterRegistry::categoryAt(int index) const {
  int seen = 0;
  for (int i = 0; i < count(); ++i) {
    if (!isFirstInCategory(i)) continue;
    if (seen == index) return chars_[i]->category();
    ++seen;
  }
  return count() ? chars_.front()->category() : "";
}

int CharacterRegistry::categoryIndexOf(const char* category) const {
  int seen = 0;
  for (int i = 0; i < count(); ++i) {
    if (!isFirstInCategory(i)) continue;
    if (std::strcmp(chars_[i]->category(), category) == 0) return seen;
    ++seen;
  }
  return 0;
}

int CharacterRegistry::countInCategory(const char* category) const {
  int n = 0;
  for (auto* c : chars_) {
    if (std::strcmp(c->category(), category) == 0) ++n;
  }
  return n;
}

Character* CharacterRegistry::inCategory(const char* category, int within) const {
  const int total = countInCategory(category);
  if (total == 0) return nullptr;
  const int target = ((within % total) + total) % total;
  int k = 0;
  for (auto* c : chars_) {
    if (std::strcmp(c->category(), category) == 0) {
      if (k == target) return c;
      ++k;
    }
  }
  return nullptr;
}

int CharacterRegistry::indexInCategory(const char* category, const std::string& id) const {
  int k = 0;
  for (auto* c : chars_) {
    if (std::strcmp(c->category(), category) == 0) {
      if (id == c->id()) return k;
      ++k;
    }
  }
  return 0;
}

}  // namespace tama
