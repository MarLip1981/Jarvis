import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components.esp32 import include_builtin_idf_component
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32"]

CONF_CS_PIN = "cs_pin"
CONF_CLK_PIN = "clk_pin"
CONF_MISO_PIN = "miso_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_FREQUENCY = "frequency"

jarvis_sd_ns = cg.esphome_ns.namespace("jarvis_sd")
JarvisSD = jarvis_sd_ns.class_("JarvisSD", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(JarvisSD),
    cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,
    cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
    cv.Required(CONF_MISO_PIN): pins.internal_gpio_input_pin_number,
    cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
    cv.Optional(CONF_FREQUENCY, default=4000000): cv.int_range(min=400000, max=20000000),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pins(config[CONF_CS_PIN], config[CONF_CLK_PIN],
                        config[CONF_MISO_PIN], config[CONF_MOSI_PIN]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))

    # ESPHome 2026.11 selectively excludes unused ESP-IDF components.
    # Re-enable the native SPI, SDSPI, SD/MMC and FATFS components used below.
    include_builtin_idf_component("esp_driver_spi")
    include_builtin_idf_component("esp_driver_sdspi")
    include_builtin_idf_component("sdmmc")
    include_builtin_idf_component("fatfs")
