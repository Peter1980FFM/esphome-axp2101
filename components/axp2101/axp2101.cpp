#include "axp2101.h"

namespace esphome {
namespace axp2101 {

static const char *const TAG = "axp2101";

void AXP2101Component::setup() {
  ESP_LOGCONFIG(TAG, "AXP2101 setup (safe mode, read-only except backlight)");
}

void AXP2101Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP2101 Power Management IC");
  LOG_I2C_DEVICE(this);
}

uint8_t AXP2101Component::read_u8(uint8_t reg) {
  uint8_t val;
  this->read_register(reg, &val, 1);
  return val;
}

uint16_t AXP2101Component::read_u12(uint8_t msb_reg) {
  uint8_t buf[2];
  this->read_register(msb_reg, buf, 2);
  return ((buf[0] << 4) | (buf[1] & 0x0F));
}

void AXP2101Component::write_u8(uint8_t reg, uint8_t value) {
  this->write_register(reg, &value, 1);
}

std::string AXP2101Component::decode_power_key(uint8_t val) {
  if (val & 0x01) return "short_press";
  if (val & 0x02) return "long_press";
  if (val & 0x04) return "very_long_press";
  if (val & 0x08) return "power_off";
  return "none";
}

void AXP2101Component::set_brightness(uint8_t brightness) {
  write_u8(0x27, brightness);
}

void AXP2101Component::update() {
  // Charging status
  if (this->battery_charging_sensor) {
    uint8_t reg01 = read_u8(0x01);
    bool charging = reg01 & 0x04;
    this->battery_charging_sensor->publish_state(charging);
  }

  // VBUS voltage
  if (this->vbus_voltage_sensor) {
    uint16_t raw = read_u12(0x5A);
    float voltage = raw * 1.7f / 1000.0f;
    this->vbus_voltage_sensor->publish_state(voltage);
  }

  // VBUS current
  if (this->vbus_current_sensor) {
    uint16_t raw = read_u12(0x5C);
    float current = raw * 0.375f;
    this->vbus_current_sensor->publish_state(current);
  }

  // PMIC temperature
  if (this->temperature_sensor) {
    uint16_t raw = read_u12(0x5E);
    float temp = raw * 0.1f - 144.7f;
    this->temperature_sensor->publish_state(temp);
  }

  // APS voltage
  if (this->aps_voltage_sensor) {
    uint16_t raw = read_u12(0x7E);
    float voltage = raw * 1.4f / 1000.0f;
    this->aps_voltage_sensor->publish_state(voltage);
  }

  // Power key events
  if (this->power_key_sensor) {
    uint8_t key = read_u8(0x46);
    std::string event = decode_power_key(key);
    this->power_key_sensor->publish_state(event);

    // Auto-reset
    if (event != "none") {
      write_u8(0x46, 0x00);
    }
  }
}

}  // namespace axp2101
}  // namespace esphome
