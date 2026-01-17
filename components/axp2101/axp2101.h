#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome {
namespace axp2101 {

class AXP2101Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  // Backlight setter
  void set_brightness(uint8_t brightness);

  // Sensor pointers
  sensor::Sensor *battery_charging_sensor{nullptr};
  sensor::Sensor *vbus_voltage_sensor{nullptr};
  sensor::Sensor *vbus_current_sensor{nullptr};
  sensor::Sensor *temperature_sensor{nullptr};
  sensor::Sensor *aps_voltage_sensor{nullptr};
  text_sensor::TextSensor *power_key_sensor{nullptr};

 protected:
  uint8_t read_u8(uint8_t reg);
  uint16_t read_u12(uint8_t msb_reg);
  void write_u8(uint8_t reg, uint8_t value);

  std::string decode_power_key(uint8_t val);
};

}  // namespace axp2101
}  // namespace esphome
