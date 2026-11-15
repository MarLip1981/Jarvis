#include "jarvis_sd.h"
#include <cstdio>
#include <cstring>
#include "esphome/core/log.h"

namespace esphome {
namespace jarvis_sd {

static const char *const TAG = "jarvis_sd";
static const char *const MOUNT_POINT = "/sdcard";
static const char *const TEST_PATH = "/sdcard/JARVIS_SD_TEST.TXT";
static const char *const TEST_TEXT = "JARVIS SD TEST OK";

void JarvisSD::set_pins(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi) {
  cs_pin_ = cs;
  clk_pin_ = clk;
  miso_pin_ = miso;
  mosi_pin_ = mosi;
}

void JarvisSD::set_frequency(uint32_t frequency) { frequency_ = frequency; }

void JarvisSD::setup() {
  // CYD screen/touch keep their existing SPI bus. The microSD socket gets
  // the separate ESP32 VSPI/SPI3 host on GPIO18/19/23 with CS on GPIO5.
  spi_bus_config_t bus_config = {};
  bus_config.mosi_io_num = mosi_pin_;
  bus_config.miso_io_num = miso_pin_;
  bus_config.sclk_io_num = clk_pin_;
  bus_config.quadwp_io_num = -1;
  bus_config.quadhd_io_num = -1;
  bus_config.max_transfer_sz = 4096;

  esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    mount_error_ = err;
    ESP_LOGE(TAG, "Nie mozna uruchomic magistrali VSPI: %s", esp_err_to_name(err));
    return;
  }

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI3_HOST;
  host.max_freq_khz = frequency_ / 1000;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.host_id = SPI3_HOST;
  slot_config.gpio_cs = static_cast<gpio_num_t>(cs_pin_);

  esp_vfs_fat_mount_config_t mount_config = {};
  mount_config.format_if_mount_failed = false;  // Nigdy nie formatuj karty automatycznie.
  mount_config.max_files = 4;
  mount_config.allocation_unit_size = 16 * 1024;

  mount_error_ = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config,
                                         &mount_config, &card_);
  if (mount_error_ != ESP_OK) {
    ESP_LOGE(TAG, "Nie udalo sie zamontowac microSD: %s", esp_err_to_name(mount_error_));
    return;
  }

  mounted_ = true;
  ESP_LOGI(TAG, "Karta microSD zamontowana przez natywne ESP-IDF SDSPI (%u Hz)",
           frequency_);

  // Bezpieczny test: dotykamy wyłącznie własnego pliku testowego.
  std::remove(TEST_PATH);
  FILE *out = std::fopen(TEST_PATH, "w");
  if (out == nullptr) {
    ESP_LOGE(TAG, "TEST BLAD: nie mozna utworzyc pliku testowego");
    return;
  }

  const size_t expected = std::strlen(TEST_TEXT);
  const size_t written = std::fwrite(TEST_TEXT, 1, expected, out);
  const int close_result = std::fclose(out);
  if (written != expected || close_result != 0) {
    ESP_LOGE(TAG, "TEST BLAD: zapis niekompletny (%u/%u bajtow)",
             static_cast<unsigned>(written), static_cast<unsigned>(expected));
    return;
  }

  FILE *in = std::fopen(TEST_PATH, "r");
  if (in == nullptr) {
    ESP_LOGE(TAG, "TEST BLAD: nie mozna otworzyc pliku do odczytu");
    return;
  }

  char actual[sizeof(TEST_TEXT)] = {};
  const size_t read_count = std::fread(actual, 1, sizeof(actual) - 1, in);
  std::fclose(in);

  if (read_count == expected && std::strcmp(actual, TEST_TEXT) == 0) {
    ESP_LOGI(TAG, "TEST OK: zapis i odczyt dzialaja; %s (%u B)",
             TEST_PATH, static_cast<unsigned>(written));
  } else {
    ESP_LOGE(TAG, "TEST BLAD: odczytana zawartosc pliku jest niezgodna");
  }
}

void JarvisSD::dump_config() {
  ESP_LOGCONFIG(TAG, "Jarvis microSD (ESP-IDF SDSPI)");
  ESP_LOGCONFIG(TAG, "  CS: GPIO%u", cs_pin_);
  ESP_LOGCONFIG(TAG, "  SCK: GPIO%u", clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO: GPIO%u", miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI: GPIO%u", mosi_pin_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(mounted_));
  if (!mounted_ && mount_error_ != ESP_FAIL) {
    ESP_LOGCONFIG(TAG, "  Error: %s", esp_err_to_name(mount_error_));
  }
}

}  // namespace jarvis_sd
}  // namespace esphome
