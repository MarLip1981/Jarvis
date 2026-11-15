#include "jarvis_sd.h"
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
  // CYD display/touch use the other ESP32 SPI host (HSPI).
  // The SD socket uses the VSPI host with its standard CYD pinout.
  spi_ = new SPIClass(VSPI);
  spi_->begin(clk_pin_, miso_pin_, mosi_pin_, cs_pin_);

  mounted_ = SD.begin(cs_pin_, *spi_, frequency_);
  if (!mounted_) {
    ESP_LOGE(TAG, "Nie udalo sie zamontowac microSD (SPI: CS=%u SCK=%u MISO=%u MOSI=%u)",
             cs_pin_, clk_pin_, miso_pin_, mosi_pin_);
    return;
  }

  ESP_LOGI(TAG, "Karta microSD zamontowana (SPI, %u Hz)", frequency_);

  SD.remove(TEST_PATH);
  File out = SD.open(TEST_PATH, FILE_WRITE);
  if (!out) {
    ESP_LOGE(TAG, "TEST BLAD: nie mozna utworzyc pliku testowego");
    return;
  }
  const size_t written = out.print(TEST_TEXT);
  out.close();
  if (written != strlen(TEST_TEXT)) {
    ESP_LOGE(TAG, "TEST BLAD: zapisano %u z %u bajtow",
             static_cast<unsigned>(written), static_cast<unsigned>(strlen(TEST_TEXT)));
    return;
  }

  File in = SD.open(TEST_PATH, FILE_READ);
  if (!in) {
    ESP_LOGE(TAG, "TEST BLAD: nie mozna otworzyc pliku do odczytu");
    return;
  }
  String actual = in.readString();
  in.close();

  if (actual == TEST_TEXT) {
    ESP_LOGI(TAG, "TEST OK: zapis i odczyt dzialaja; plik %s (%u B)",
             TEST_PATH, static_cast<unsigned>(written));
  } else {
    ESP_LOGE(TAG, "TEST BLAD: odczytana zawartosc pliku jest niezgodna");
  }
}

void JarvisSD::dump_config() {
  ESP_LOGCONFIG(TAG, "Jarvis microSD (SPI)");
  ESP_LOGCONFIG(TAG, "  CS: GPIO%u", cs_pin_);
  ESP_LOGCONFIG(TAG, "  SCK: GPIO%u", clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO: GPIO%u", miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI: GPIO%u", mosi_pin_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", YESNO(mounted_));
}

}  // namespace jarvis_sd
}  // namespace esphome
