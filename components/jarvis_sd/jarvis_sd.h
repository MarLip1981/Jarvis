#pragma once

#include "esphome/core/component.h"
#include <SPI.h>

namespace esphome {
namespace jarvis_sd {

class JarvisSD : public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_pins(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi);
  void set_frequency(uint32_t frequency);
  bool is_mounted() const { return mounted_; }
  size_t free_heap_before() const { return free_heap_before_; }
  size_t free_heap_after() const { return free_heap_after_; }
  size_t largest_block_before() const { return largest_block_before_; }
  size_t largest_block_after() const { return largest_block_after_; }

 protected:
  uint8_t cs_pin_{5};
  uint8_t clk_pin_{18};
  uint8_t miso_pin_{19};
  uint8_t mosi_pin_{23};
  uint32_t frequency_{4000000};
  SPIClass spi_sd_{VSPI};
  bool mounted_{false};
  size_t free_heap_before_{0};
  size_t free_heap_after_{0};
  size_t largest_block_before_{0};
  size_t largest_block_after_{0};
};

}  // namespace jarvis_sd
}  // namespace esphome
