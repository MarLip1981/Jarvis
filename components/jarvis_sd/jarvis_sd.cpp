#include "jarvis_sd.h"
#include <Arduino.h>
#include <SD.h>
#include "esp_heap_caps.h"
#include "esphome/core/log.h"

namespace esphome {
namespace jarvis_sd {

static const char *const TAG = "jarvis_sd";

static void log_heap_stats(const char *stage) {
  const size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const size_t largest_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  ESP_LOGI(TAG, "RAM %s: wolny heap=%u B, najwiekszy blok=%u B",
           stage, static_cast<unsigned>(free_heap),
           static_cast<unsigned>(largest_block));
}

void JarvisSD::set_pins(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi) {
  cs_pin_ = cs;
  clk_pin_ = clk;
  miso_pin_ = miso;
  mosi_pin_ = mosi;
}

void JarvisSD::set_frequency(uint32_t frequency) { frequency_ = frequency; }

void JarvisSD::setup() {
  free_heap_before_ = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  largest_block_before_ = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  log_heap_stats("przed montowaniem SD");

  // Karta korzysta z osobnej magistrali VSPI; ekran i dotyk pozostaja bez zmian.
  pinMode(cs_pin_, OUTPUT);
  digitalWrite(cs_pin_, HIGH);
  spi_sd_.begin(clk_pin_, miso_pin_, mosi_pin_, cs_pin_);

  // Dwa deskryptory wystarcza na rownoczesny plik audio i grafike; domyslnie jest piec.
  if (!SD.begin(cs_pin_, spi_sd_, frequency_, "/sd", 2, false)) {
    mounted_ = false;
    ESP_LOGE(TAG, "Nie udalo sie zamontowac karty microSD");
    free_heap_after_ = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    largest_block_after_ = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    log_heap_stats("po nieudanym montowaniu SD");
    return;
  }

  mounted_ = true;
  free_heap_after_ = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  largest_block_after_ = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  ESP_LOGI(TAG, "Karta microSD zamontowana (dwa pliki, bez formatowania)");
  log_heap_stats("po zamontowaniu SD");
}

void JarvisSD::unmount() {
  if (mounted_) {
    // Zamykamy system plikow dopiero po zakonczeniu diagnostyki.
    SD.end();
    mounted_ = false;
    ESP_LOGI(TAG, "Karta microSD odmontowana");
  } else {
    ESP_LOGW(TAG, "Odmontowanie SD: karta nie byla zamontowana");
  }

  // Zwalniamy rowniez magistrale VSPI uzywana wylacznie przez karte.
  spi_sd_.end();
  digitalWrite(cs_pin_, HIGH);
  was_mounted_ = was_mounted_ || free_heap_after_ > 0;
  free_heap_after_unmount_ = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  largest_block_after_unmount_ = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  log_heap_stats("po odmontowaniu SD");
}

void JarvisSD::dump_config() {
  ESP_LOGCONFIG(TAG, "Jarvis microSD (Arduino SD / VSPI, mount/unmount)");
  ESP_LOGCONFIG(TAG, "  CS: GPIO%u", cs_pin_);
  ESP_LOGCONFIG(TAG, "  SCK: GPIO%u", clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO: GPIO%u", miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI: GPIO%u", mosi_pin_);
  ESP_LOGCONFIG(TAG, "  Frequency: %u Hz", frequency_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(mounted_));
}

}  // namespace jarvis_sd
}  // namespace esphome
