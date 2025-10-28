#pragma once
#include <Arduino.h>
#include "Appliance/ApplianceBase.h"
#include "Appliance/AirConditioner/Capabilities.h"
#include "Appliance/AirConditioner/StatusData.h"
#include "Appliance/AirConditioner/DiagData.h"
#include "Helpers/Helpers.h"

namespace dudanov {
namespace midea {
namespace ac {

// Air conditioner control command
struct Control {
  Optional<float> targetTemp{};
  Optional<Mode> mode{};
  Optional<Preset> preset{};
  Optional<FanMode> fanMode{};
  Optional<SwingMode> swingMode{};
};

class AirConditioner : public ApplianceBase {
 public:
  AirConditioner() : ApplianceBase(AIR_CONDITIONER) {}
  void m_setup() override;
  void m_onIdle() override { this->m_runSeq(); }
  //void m_onIdle2() override {this->m_getDiag1();this->m_getDiag2(); }
  void control(const Control &control);
  void setPowerState(bool state);
  bool getPowerState() const { return this->m_mode != Mode::MODE_OFF; }
  void togglePowerState() { this->setPowerState(this->m_mode == Mode::MODE_OFF); }
  float getTargetTemp() const { return this->m_targetTemp; }
  float getIndoorTemp() const { return this->m_indoorTemp; }
  float getOutdoorTemp() const { return this->m_outdoorTemp; }
  float getT1Temp() const { return this->m_t1Temp; }
  float getT2Temp() const { return this->m_t2Temp; }
  float getT3Temp() const { return this->m_t3Temp; }
  float getT4Temp() const { return this->m_t4Temp; }
  float getEEV() const { return this->m_eev; }
  float getIndoorHum() const { return this->m_indoorHumidity; }
  float getEnergyUsage() const { return this->m_energyUsage; }
  float getPowerUsage() const { return this->m_powerUsage; }
  float getCompressorTarget() const { return this->m_compressorTarget; }
  float getCompressorSpeed() const { return this->m_compressorSpeed; }
  float getIdFTarget() const { return this->m_idFTarget; }
  float getIdFVal() const { return this->m_idFVal; }
  float getOdFVal() const { return this->m_odFVal; }
  float getRunMode() const { return this->m_runMode; }
  bool  getDefrost() const { return this->m_defrost; }
  float getVal1_8() const { return this->m_val1_8; }
  float getVal2_12() const { return this->m_val2_12; }
  Mode getMode() const { return this->m_mode; }
  SwingMode getSwingMode() const { return this->m_swingMode; }
  FanMode getFanMode() const { return this->m_fanMode; }
  Preset getPreset() const { return this->m_preset; }
  const Capabilities &getCapabilities() const { return this->m_capabilities; }
  void displayToggle() { this->m_displayToggle(); }
 protected:
  void m_getEnergyUsage();
  void m_getPowerUsage();
  void m_getCapabilities();
  void m_getStatus();
  void m_setStatus(StatusData status);
  void m_displayToggle();
  void m_getDiag1();
  void m_getDiag2();
  void m_getDiag3();
  void m_runSeq();
  ResponseStatus m_readStatus(FrameData data);
  ResponseStatus m_readDiag1(FrameData data);
  ResponseStatus m_readDiag2(FrameData data);
  ResponseStatus m_readDiag3(FrameData data);
  Capabilities m_capabilities{};
  Timer m_powerUsageTimer;
  float m_indoorHumidity{};
  float m_indoorTemp{};
  float m_outdoorTemp{};
  float m_t1Temp{};
  float m_t2Temp{};
  float m_t3Temp{};
  float m_t4Temp{};
  float m_eev{};
  float m_runMode{};
  float m_defrost{};
  float m_val1_8{};
  float m_val2_12{};
  float m_targetTemp{};
  float m_powerUsage{};
  float m_energyUsage{};
  float m_compressorTarget{};
  float m_compressorSpeed{};
  float m_idFTarget{};
  float m_idFVal{};
  float m_odFVal{};
  Mode m_mode{Mode::MODE_OFF};
  Preset m_preset{Preset::PRESET_NONE};
  FanMode m_fanMode{FanMode::FAN_AUTO};
  SwingMode m_swingMode{SwingMode::SWING_OFF};
  Preset m_lastPreset{Preset::PRESET_NONE};
  StatusData m_status{};
  DiagData m_diag{};
  bool m_sendControl{};
  uint8_t m_seq{};
};

}  // namespace ac
}  // namespace midea
}  // namespace dudanov
