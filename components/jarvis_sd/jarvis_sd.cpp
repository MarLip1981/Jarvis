#include "jarvis_sd.h"
#include <cstdio>
#include <cstring>
#include "esphome/core/log.h"

namespace esphome {
namespace jarvis_sd {

static const char *const TAG = "jarvis_sd";
static const char *const TEST_PATH = "/JARVIS_SD_TEST.TXT";
static const char *const TEST_TEXT = "JARVIS SD TEST OK";

void JarvisSD::set_pins(uint8_t cs, uint8_t clk, uint8_t miso, uint8_t mosi) {
  cs_pin_ = cs;
  clk_pin_ = clk;
  miso_pin_ = miso;
  mosi_pin_ = mosi;
}

void JarvisSD::set_frequency(uint32_t frequency) { frequency_ = frequency; }

void JarvisSD::setup() {
  // Marauder CYD GUITION: prepare SD CS HIGH, then mount with Arduino SD
  // on the ESP32 default VSPI bus. GPIO18/19/23 are the CYD SD SPI lines.
  pinMode(cs_pin_, OUTPUT);
  digitalWrite(cs_pin_, HIGH);
  delay(10);

  ESP_LOGW(TAG, "DIAG SD: inicjalizacja jak w Marauderze: CS=GPIO%u, VSPI SCK=%u MISO=%u MOSI=%u",
           cs_pin_, clk_pin_, miso_pin_, mosi_pin_);

  spi_sd_.begin(clk_pin_, miso_pin_, mosi_pin_, cs_pin_);
  delay(10);

  if (!SD.begin(cs_pin_, spi_sd_, frequency_)) {
    ESP_LOGE(TAG, "DIAG SD: SD.begin nie powiodlo sie (CS=GPIO%u, %u Hz)",
             cs_pin_, frequency_);
    mounted_ = false;
    return;
  }

  mounted_ = true;
  const uint8_t card_type = SD.cardType();
  const uint64_t card_size = SD.cardSize();

  if (card_type == CARD_NONE || card_size == 0) {
    ESP_LOGE(TAG, "DIAG SD: SD.begin zwrocilo sukces, ale karta nie podaje typu/pojemnosci");
    mounted_ = false;
    SD.end();
    return;
  }

  ESP_LOGW(TAG, "DIAG SD: karta rozpoznana; typ=%u, pojemnosc=%llu MB",
           card_type, static_cast<unsigned long long>(card_size / (1024ULL * 1024ULL)));

  // Safe filesystem test: touch only the dedicated Jarvis test file.
  SD.remove(TEST_PATH);
  File out = SD.open(TEST_PATH, FILE_WRITE);
  if (!out) {
    ESP_LOGE(TAG, "DIAG SD: nie mozna utworzyc pliku testowego");
    return;
  }

  const size_t expected = std::strlen(TEST_TEXT);
  const size_t written = out.write(reinterpret_cast<const uint8_t *>(TEST_TEXT), expected);
  out.flush();
  out.close();

  if (written != expected) {
    ESP_LOGE(TAG, "DIAG SD: zapis niekompletny (%u/%u bajtow)",
             static_cast<unsigned>(written), static_cast<unsigned>(expected));
    return;
  }

  File in = SD.open(TEST_PATH, FILE_READ);
  if (!in) {
    ESP_LOGE(TAG, "DIAG SD: nie mozna otworzyc pliku testowego do odczytu");
    return;
  }

  char actual[sizeof(TEST_TEXT)] = {};
  const size_t read_count = in.readBytes(actual, sizeof(actual) - 1);
  in.close();

  if (read_count == expected && std::strcmp(actual, TEST_TEXT) == 0) {
    ESP_LOGW(TAG, "DIAG SD: TEST OK - zapis i odczyt dzialaja (%u B)",
             static_cast<unsigned>(written));
  } else {
    ESP_LOGE(TAG, "DIAG SD: TEST BLAD - odczytana zawartosc pliku jest niezgodna");
  }
}

void JarvisSD::dump_config() {
  ESP_LOGCONFIG(TAG, "Jarvis microSD (Arduino SD / VSPI, tryb zgodny z Marauderem)");
  ESP_LOGCONFIG(TAG, "  CS: GPIO%u", cs_pin_);
  ESP_LOGCONFIG(TAG, "  SCK: GPIO%u", clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO: GPIO%u", miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI: GPIO%u", mosi_pin_);
  ESP_LOGCONFIG(TAG, "  Frequency: %u Hz", frequency_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(mounted_));
}

}  // namespace jarvis_sd
}  // namespace esphome
