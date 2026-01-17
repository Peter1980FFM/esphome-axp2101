#pragma once

#include "esphome/components/sensor/sensor.h"
#include "axp2101.h"

namespace esphome {
namespace axp2101 {

class AXP2101Component;

class AXP2101ChargingSensor : public sensor::Sensor {
 public:
  void set_parent(AXP2101Component *parent) { parent_ = parent; }
 protected:
  AXP2101Component *parent_;
};

class AXP2101VBUSVoltageSensor : public sensor::Sensor {
 public:
  void set_parent(AXP2101Component *parent) { parent_ = parent; }
 protected:
  AXP2101Component *parent_;
};

class AXP2101VBUSCurrentSensor : public sensor::Sensor {
 public:
  void set_parent(AXP2101Component *parent) { parent_ = parent; }
 protected:
  AXP2101Component *parent_;
};

class AXP2101TemperatureSensor : public sensor::Sensor {
 public:
  void set_parent(AXP2101Component *parent) { parent_ = parent; }
 protected:
  AXP2101Component *parent_;
};

class AXP2101APSVoltageSensor : public sensor::Sensor {
 public:
  void set_parent(AXP2101Component *parent) { parent_ = parent; }
 protected:
  AXP2101Component *parent_;
};

}  // namespace axp2101
}  // namespace esphome
