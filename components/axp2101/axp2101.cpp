#warning "AXP2101 FROM PETER1980FFM FORK LOADED"
#include "axp2101.h"
#include "esp_sleep.h"
#include "esphome/core/log.h"

#ifndef CONFIG_PMU_SDA
#define CONFIG_PMU_SDA 21
#endif

#ifndef CONFIG_PMU_SCL
#define CONFIG_PMU_SCL 22
#endif

#ifndef CONFIG_PMU_IRQ
#define CONFIG_PMU_IRQ 35
#endif

namespace esphome {
namespace axp2101 {

static const char *TAG = "axp2101.sensor";

void AXP2101Component::setup() {
  ESP_LOGCONFIG(TAG, "AXP2101 setup, startup reason: %s", GetStartupReason().c_str());

  ESP_LOGCONFIG(TAG, "Enabling AXP2101 ADC engine");
  Write1Byte(0x82, 0xFF);   // Enable all ADC channels
  Write1Byte(0x84, 0x00);   // ADC sample rate = 25Hz
  
  uint8_t pwr = Read8bit(0x12);
  Write1Byte(0x12, pwr | 0x01);  // Enable DCDC1// Hier bewusst KEIN XPowersLib, KEIN eigener I2C-Treiber.
  
  Write1Byte(0x82, Read8bit(0x82) | 0x80);
}

void AXP2101Component::dump_config() {
  ESP_LOGCONFIG(TAG, "AXP2101 (light, no XPowersLib):");
  LOG_I2C_DEVICE(this);
  LOG_SENSOR("  ", "Battery Voltage", this->batteryvoltage_sensor_);
  LOG_SENSOR("  ", "Battery Level", this->batterylevel_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery Charging", this->batterycharging_bsensor_);
}

float AXP2101Component::get_setup_priority() const { return setup_priority::DATA; }

void AXP2101Component::update() {
  // Spannung + Level aus den Rohregistern (wie in deinen Helpern)
  if (this->batterylevel_sensor_ != nullptr || this->batteryvoltage_sensor_ != nullptr) {
    uint16_t raw_vbat = GetVbatData();  // 12-bit Wert aus 0x78/0x79
    ESP_LOGD(TAG, "AXP2101 RAW VBAT = 0x%04X", raw_vbat);

    // Grobe Skalierung: AXP21xx nutzt typischerweise ~1.1mV/LSB
    float vbat_mv = raw_vbat * 1.1f;
    float vbat_v = vbat_mv / 1000.0f;

    ESP_LOGD(TAG, "Raw Vbat=0x%04X -> %f V", raw_vbat, vbat_v);

    if (this->batteryvoltage_sensor_ != nullptr) {
      this->batteryvoltage_sensor_->publish_state(vbat_v);
    }

    if (this->batterylevel_sensor_ != nullptr) {
      float batterylevel;
      if (vbat_v <= 3.0f)
        batterylevel = 0.0f;
      else if (vbat_v >= 4.1f)
        batterylevel = 100.0f;
      else
        batterylevel = 100.0f * ((vbat_v - 3.0f) / (4.1f - 3.0f));

      if (batterylevel > 100.0f)
        batterylevel = 100.0f;
      if (batterylevel < 0.0f)
        batterylevel = 0.0f;

      ESP_LOGD(TAG, "Battery Level approx=%f %%", batterylevel);
      this->batterylevel_sensor_->publish_state(batterylevel);
    }
  }

  if (this->batterycharging_bsensor_ != nullptr) {
    // Sehr einfache Heuristik: Bit in Status-Register 0x01 auswerten
    uint8_t status = Read8bit(0x01);
    bool charging = (status & 0x04) != 0;  // Bitmaske ggf. anpassen, wenn du das echte Bit kennst
    ESP_LOGD(TAG, "Battery Charging (heuristic)=%s (status=0x%02X)", charging ? "true" : "false", status);
    this->batterycharging_bsensor_->publish_state(charging);
  }

  UpdateBrightness();
}

void AXP2101Component::Write1Byte(uint8_t Addr, uint8_t Data) {
  this->write_byte(Addr, Data);
}

uint8_t AXP2101Component::Read8bit(uint8_t Addr) {
  uint8_t data;
  this->read_byte(Addr, &data);
  return data;
}

uint16_t AXP2101Component::Read12Bit(uint8_t Addr) {
  uint16_t Data = 0;
  uint8_t buf[2];
  ReadBuff(Addr, 2, buf);
  Data = ((buf[0] << 4) + buf[1]);
  return Data;
}

uint16_t AXP2101Component::Read13Bit(uint8_t Addr) {
  uint16_t Data = 0;
  uint8_t buf[2];
  ReadBuff(Addr, 2, buf);
  Data = ((buf[0] << 5) + buf[1]);
  return Data;
}

uint16_t AXP2101Component::Read16bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[2];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < (int) sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

uint32_t AXP2101Component::Read24bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[3];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < (int) sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

uint32_t AXP2101Component::Read32bit(uint8_t Addr) {
  uint32_t ReData = 0;
  uint8_t Buff[4];
  this->read_bytes(Addr, Buff, sizeof(Buff));
  for (int i = 0; i < (int) sizeof(Buff); i++) {
    ReData <<= 8;
    ReData |= Buff[i];
  }
  return ReData;
}

void AXP2101Component::ReadBuff(uint8_t Addr, uint8_t Size, uint8_t *Buff) {
  this->read_bytes(Addr, Buff, Size);
}

void AXP2101Component::UpdateBrightness() {
  if (brightness_ == curr_brightness_) {
    return;
  }

  ESP_LOGD(TAG, "Brightness=%f (Curr: %f)", brightness_, curr_brightness_);
  curr_brightness_ = brightness_;

  const uint8_t c_min = 7;
  const uint8_t c_max = 12;
  auto ubri = c_min + static_cast<uint8_t>(brightness_ * (c_max - c_min));

  if (ubri > c_max) {
    ubri = c_max;
  }
  switch (this->model_) {
    case AXP2101_M5CORE2: {
      uint8_t buf = Read8bit(0x27);
      Write1Byte(0x27, ((buf & 0x80) | (ubri << 3)));
      break;
    }
  }
}

bool AXP2101Component::GetBatState() {
  if (Read8bit(0x01) | 0x20)
    return true;
  else
    return false;
}

uint8_t AXP2101Component::GetBatData() {
  return Read8bit(0x75);
}

// Coulombcounter-Funktionen bleiben als Wrapper auf die Register,
// werden aktuell aber in Phase 1 nicht aktiv genutzt.
void AXP2101Component::EnableCoulombcounter(void) {
  Write1Byte(0xB8, 0x80);
}

void AXP2101Component::DisableCoulombcounter(void) {
  Write1Byte(0xB8, 0x00);
}

void AXP2101Component::StopCoulombcounter(void) {
  Write1Byte(0xB8, 0xC0);
}

void AXP2101Component::ClearCoulombcounter(void) {
  Write1Byte(0xB8, 0xA0);
}

uint32_t AXP2101Component::GetCoulombchargeData(void) {
  return Read32bit(0xB0);
}

uint32_t AXP2101Component::GetCoulombdischargeData(void) {
  return Read32bit(0xB4);
}

float AXP2101Component::GetCoulombData(void) {
  uint32_t coin = GetCoulombchargeData();
  uint32_t coout = GetCoulombdischargeData();
  float ccc = 65536 * 0.5f * (coin - coout) / 3600.0f / 25.0f;
  return ccc;
}

uint16_t AXP2101Component::GetVbatData(void) {
  uint16_t vbat = 0;
  uint8_t buf[2];
  ReadBuff(0x78, 2, buf);
  vbat = ((buf[0] << 4) + buf[1]);
  return vbat;
}

uint16_t AXP2101Component::GetVinData(void) {
  uint16_t vin = 0;
  uint8_t buf[2];
  ReadBuff(0x56, 2, buf);
  vin = ((buf[0] << 4) + buf[1]);
  return vin;
}

uint16_t AXP2101Component::GetIinData(void) {
  uint16_t iin = 0;
  uint8_t buf[2];
  ReadBuff(0x58, 2, buf);
  iin = ((buf[0] << 4) + buf[1]);
  return iin;
}

uint16_t AXP2101Component::GetVusbinData(void) {
  uint16_t vin = 0;
  uint8_t buf[2];
  ReadBuff(0x5a, 2, buf);
  vin = ((buf[0] << 4) + buf[1]);
  return vin;
}

uint16_t AXP2101Component::GetIusbinData(void) {
  uint16_t iin = 0;
  uint8_t buf[2];
  ReadBuff(0x5C, 2, buf);
  iin = ((buf[0] << 4) + buf[1]);
  return iin;
}

uint16_t AXP2101Component::GetIchargeData(void) {
  uint16_t icharge = 0;
  uint8_t buf[2];
  ReadBuff(0x7A, 2, buf);
  icharge = (buf[0] << 5) + buf[1];
  return icharge;
}

uint16_t AXP2101Component::GetIdischargeData(void) {
  uint16_t idischarge = 0;
  uint8_t buf[2];
  ReadBuff(0x7C, 2, buf);
  idischarge = (buf[0] << 5) + buf[1];
  return idischarge;
}

uint16_t AXP2101Component::GetTempData(void) {
  uint16_t temp = 0;
  uint8_t buf[2];
  ReadBuff(0x5e, 2, buf);
  temp = ((buf[0] << 4) + buf[1]);
  return temp;
}

uint32_t AXP2101Component::GetPowerbatData(void) {
  uint32_t power = 0;
  uint8_t buf[3];
  ReadBuff(0x70, 2, buf);
  power = (buf[0] << 16) + (buf[1] << 8) + buf[2];
  return power;
}

uint16_t AXP2101Component::GetVapsData(void) {
  uint16_t vaps = 0;
  uint8_t buf[2];
  ReadBuff(0x7e, 2, buf);
  vaps = ((buf[0] << 4) + buf[1]);
  return vaps;
}

void AXP2101Component::SetSleep(void) {
  Write1Byte(0x31, Read8bit(0x31) | (1 << 3));
  Write1Byte(0x90, Read8bit(0x90) | 0x07);
  Write1Byte(0x82, 0x00);
  Write1Byte(0x12, Read8bit(0x12) & 0xA1);
}

void AXP2101Component::DeepSleep(uint64_t time_in_us) {
  SetSleep();
  esp_sleep_enable_ext0_wakeup((gpio_num_t)37, 0 /* LOW */);
  if (time_in_us > 0) {
    esp_sleep_enable_timer_wakeup(time_in_us);
  } else {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  }
  (time_in_us == 0) ? esp_deep_sleep_start() : esp_deep_sleep(time_in_us);
}

void AXP2101Component::LightSleep(uint64_t time_in_us) {
  if (time_in_us > 0) {
    esp_sleep_enable_timer_wakeup(time_in_us);
  } else {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  }
  esp_light_sleep_start();
}

uint8_t AXP2101Component::GetBtnPress() {
  uint8_t state = Read8bit(0x46);
  if (state) {
    Write1Byte(0x46, 0x03);
  }
  return state;
}

uint8_t AXP2101Component::GetWarningLevel(void) {
  return Read8bit(0x47) & 0x01;
}

float AXP2101Component::GetBatCurrent() {
  float ADCLSB = 0.5f;
  uint16_t CurrentIn = Read13Bit(0x7A);
  uint16_t CurrentOut = Read13Bit(0x7C);
  return (CurrentIn - CurrentOut) * ADCLSB;
}

float AXP2101Component::GetVinVoltage() {
  float ADCLSB = 1.7f / 1000.0f;
  uint16_t ReData = Read12Bit(0x56);
  return ReData * ADCLSB;
}

float AXP2101Component::GetVinCurrent() {
  float ADCLSB = 0.625f;
  uint16_t ReData = Read12Bit(0x58);
  return ReData * ADCLSB;
}

float AXP2101Component::GetVBusVoltage() {
  float ADCLSB = 1.7f / 1000.0f;
  uint16_t ReData = Read12Bit(0x5A);
  return ReData * ADCLSB;
}

float AXP2101Component::GetVBusCurrent() {
  float ADCLSB = 0.375f;
  uint16_t ReData = Read12Bit(0x5C);
  return ReData * ADCLSB;
}

float AXP2101Component::GetTempInAXP2101() {
  float ADCLSB = 0.1f;
  const float OFFSET_DEG_C = -144.7f;
  uint16_t ReData = Read12Bit(0x5E);
  return OFFSET_DEG_C + ReData * ADCLSB;
}

float AXP2101Component::GetBatPower() {
  float VoltageLSB = 1.1f;
  float CurrentLCS = 0.5f;
  uint32_t ReData = Read24bit(0x70);
  return VoltageLSB * CurrentLCS * ReData / 1000.0f;
}

float AXP2101Component::GetBatChargeCurrent() {
  float ADCLSB = 0.5f;
  uint16_t ReData = Read13Bit(0x7A);
  return ReData * ADCLSB;
}

float AXP2101Component::GetAPSVoltage() {
  float ADCLSB = 1.4f / 1000.0f;
  uint16_t ReData = Read12Bit(0x7E);
  return ReData * ADCLSB;
}

float AXP2101Component::GetBatCoulombInput() {
  uint32_t ReData = Read32bit(0xB0);
  return ReData * 65536 * 0.5f / 3600.0f / 25.0f;
}

float AXP2101Component::GetBatCoulombOut() {
  uint32_t ReData = Read32bit(0xB4);
  return ReData * 65536 * 0.5f / 3600.0f / 25.0f;
}

void AXP2101Component::SetCoulombClear() {
  Write1Byte(0xB8, 0x20);
}

void AXP2101Component::SetLDO2(bool State) {
  uint8_t buf = Read8bit(0x12);
  if (State == true) {
    buf = (1 << 2) | buf;
  } else {
    buf = ~(1 << 2) & buf;
  }
  Write1Byte(0x12, buf);
}

void AXP2101Component::SetLDO3(bool State) {
  uint8_t buf = Read8bit(0x12);
  if (State == true) {
    buf = (1 << 3) | buf;
  } else {
    buf = ~(1 << 3) & buf;
  }
  Write1Byte(0x12, buf);
}

void AXP2101Component::SetChargeCurrent(uint8_t current) {
  uint8_t buf = Read8bit(0x33);
  buf = (buf & 0xf0) | (current & 0x07);
  Write1Byte(0x33, buf);
}

void AXP2101Component::PowerOff() {
  Write1Byte(0x32, Read8bit(0x32) | 0x80);
}

void AXP2101Component::SetAdcState(bool state) {
  Write1Byte(0x82, state ? 0xff : 0x00);
}

std::string AXP2101Component::GetStartupReason() {
  esp_reset_reason_t reset_reason = ::esp_reset_reason();
  if (reset_reason == ESP_RST_DEEPSLEEP) {
    esp_sleep_source_t wake_reason = esp_sleep_get_wakeup_cause();
    if (wake_reason == ESP_SLEEP_WAKEUP_EXT0)
      return "ESP_SLEEP_WAKEUP_EXT0";
    if (wake_reason == ESP_SLEEP_WAKEUP_EXT1)
      return "ESP_SLEEP_WAKEUP_EXT1";
    if (wake_reason == ESP_SLEEP_WAKEUP_TIMER)
      return "ESP_SLEEP_WAKEUP_TIMER";
    if (wake_reason == ESP_SLEEP_WAKEUP_TOUCHPAD)
      return "ESP_SLEEP_WAKEUP_TOUCHPAD";
    if (wake_reason == ESP_SLEEP_WAKEUP_ULP)
      return "ESP_SLEEP_WAKEUP_ULP";
    if (wake_reason == ESP_SLEEP_WAKEUP_GPIO)
      return "ESP_SLEEP_WAKEUP_GPIO";
    if (wake_reason == ESP_SLEEP_WAKEUP_UART)
      return "ESP_SLEEP_WAKEUP_UART";
    return std::string{"WAKEUP_UNKNOWN_REASON"};
  }

  if (reset_reason == ESP_RST_UNKNOWN)
    return "ESP_RST_UNKNOWN";
  if (reset_reason == ESP_RST_POWERON)
    return "ESP_RST_POWERON";
  if (reset_reason == ESP_RST_SW)
    return "ESP_RST_SW";
  if (reset_reason == ESP_RST_PANIC)
    return "ESP_RST_PANIC";
  if (reset_reason == ESP_RST_INT_WDT)
    return "ESP_RST_INT_WDT";
  if (reset_reason == ESP_RST_TASK_WDT)
    return "ESP_RST_TASK_WDT";
  if (reset_reason == ESP_RST_WDT)
    return "ESP_RST_WDT";
  if (reset_reason == ESP_RST_BROWNOUT)
    return "ESP_RST_BROWNOUT";
  if (reset_reason == ESP_RST_SDIO)
    return "ESP_RST_SDIO";
  return std::string{"RESET_UNKNOWN_REASON"};
}

}  // namespace axp2101
}  // namespace esphome
