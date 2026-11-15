#pragma once

#include "esphome/core/component.h"
#include "driver/spi_master.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

namespace esphome {
namespace jarvis_sd {

class JarvisSD : public Component {
 public:
  void setup() override;
  void dump_config() override;
  void set_pins(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi);
  void set_frequency(uint32_t frequency);
  bool is_mounted() const { return mounted_; }

 protected:
  uint8_t cs_pin_{5};
  uint8_t clk_pin_{18};
  uint8_t miso_pin_{19};
  uint8_t mosi_pin_{23};
  uint32_t frequency_{4000000};
  sdmmc_card_t *card_{nullptr};
  bool mounted_{false};
  esp_err_t mount_error_{ESP_FAIL};
};

}  // namespace jarvis_sd
}  // namespace esphome
