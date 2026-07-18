#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "gfx.h"

namespace tama::logos {

bool has(const std::string& id);

int width(int height);

bool draw(Gfx& g, const std::string& id, int cx, int cy, int size, uint16_t col);

void setRuntime(const std::string& id, int w, int h, const std::vector<uint8_t>& bits);

}  // namespace tama::logos
