#pragma once

#include <esp_partition.h>

#include <string>

#include "config.h"

namespace tama {

class M5Config : public config::ISource {
 public:
  std::string read() override {
    const esp_partition_t* part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, static_cast<esp_partition_subtype_t>(0x40), "tamacfg");
    if (!part) return {};

    uint8_t header[config::kHeaderSize] = {0};
    if (esp_partition_read(part, 0, header, sizeof(header)) != ESP_OK) return {};

    const size_t len = static_cast<size_t>(header[4]) | (static_cast<size_t>(header[5]) << 8);
    if (len == 0 || sizeof(header) + len > part->size) return {};

    std::string blob;
    blob.resize(sizeof(header) + len);
    if (esp_partition_read(part, 0, blob.data(), blob.size()) != ESP_OK) return {};
    return blob;
  }
};

}  // namespace tama
