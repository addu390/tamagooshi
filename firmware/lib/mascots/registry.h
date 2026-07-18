#pragma once

#include <string>
#include <vector>

#include "character.h"

namespace tama {

class CharacterRegistry {
 public:
  void add(Character& character);
  Character* get(const std::string& id) const;
  Character* getOrDefault(const std::string& id) const;
  Character* first() const;
  Character* byIndex(int index) const;
  int indexOf(const std::string& id) const;
  int count() const { return static_cast<int>(chars_.size()); }

  int categoryCount() const;
  const char* categoryAt(int index) const;
  int categoryIndexOf(const char* category) const;
  int countInCategory(const char* category) const;
  Character* inCategory(const char* category, int within) const;
  int indexInCategory(const char* category, const std::string& id) const;

 private:
  bool isFirstInCategory(int index) const;

  std::vector<Character*> chars_;
};

}  // namespace tama
