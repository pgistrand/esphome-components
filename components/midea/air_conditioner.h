#pragma once

#ifdef USE_ARDUINO

// MideaUART
#include <Appliance/AirConditioner/AirConditioner.h>

#include "appliance_base.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace midea {
namespace ac {

using binary_sensor::BinarySensor;
using sensor::Sensor;
using climate::ClimateCall;
using climate::ClimatePreset;
using climate::ClimateTraits;
using climate::ClimateMode;
using climate::ClimateSwingMode;
using climate::ClimateFanMode;

class AirConditioner : public ApplianceBase<dudanov::midea::ac::AirConditioner>, public climate::Climate {
 public:
  void dump_config() override;
  void set_outdoor_temperature_sensor(Sensor *sensor) { this->outdoor_sensor_ = sensor; }
  void set_t1_temperature_sensor(Sensor *sensor) { this->t1_sensor_ = sensor; }
  void set_t2_temperature_sensor(Sensor *sensor) { this->t2_sensor_ = sensor; }
  void set_t3_temperature_sensor(Sensor *sensor) { this->t3_sensor_ = sensor; }
  void set_t4_temperature_sensor(Sensor *sensor) { this->t4_sensor_ = sensor; }
  void set_eev_sensor(Sensor *sensor) { this->eev_sensor_ = sensor; }
  void set_humidity_setpoint_sensor(Sensor *sensor) { this->humidity_sensor_ = sensor; }
  void set_power_sensor(Sensor *sensor) { this->power_sensor_ = sensor; }
  void set_energy_sensor(Sensor *sensor) { this->energy_sensor_ = sensor; }
  void set_compressor_target_sensor(Sensor *sensor) { this->compressor_target_sensor_ = sensor; }
  void set_compressor_value_sensor(Sensor *sensor) { this->compressor_value_sensor_ = sensor; }
  void set_run_mode_sensor(Sensor *sensor) { this->run_mode_sensor_ = sensor; }
  void set_defrost_sensor(BinarySensor *sensor) { this->defrost_sensor_ = sensor; }
  void set_val1_8_sensor(Sensor *sensor) { this->val1_8_sensor_ = sensor; }
  void set_val2_12_sensor(Sensor *sensor) { this->val2_12_sensor_ = sensor; }
  void set_idFTarget_sensor(Sensor *sensor) { this->idFTarget_sensor_ = sensor; }
  void set_idFVal_sensor(Sensor *sensor) { this->idFVal_sensor_ = sensor; }
  void set_odFVal_sensor(Sensor *sensor) { this->odFVal_sensor_ = sensor; }
  void on_status_change() override;

  /* ############### */
  /* ### ACTIONS ### */
  /* ############### */

  void do_follow_me(float temperature, bool use_fahrenheit, bool beeper = false);
  void do_display_toggle();
  void do_swing_step();
  void do_beeper_on() { this->set_beeper_feedback(true); }
  void do_beeper_off() { this->set_beeper_feedback(false); }
  void do_power_on() { this->base_.setPowerState(true); }
  void do_power_off() { this->base_.setPowerState(false); }
  void do_power_toggle() { this->base_.setPowerState(this->mode == ClimateMode::CLIMATE_MODE_OFF); }
  void set_supported_modes(const std::set<ClimateMode> &modes) { this->supported_modes_ = modes; }
  void set_supported_swing_modes(const std::set<ClimateSwingMode> &modes) { this->supported_swing_modes_ = modes; }
  void set_supported_presets(const std::set<ClimatePreset> &presets) { this->supported_presets_ = presets; }
  void set_custom_presets(const std::set<std::string> &presets) { this->supported_custom_presets_ = presets; }
  void set_custom_fan_modes(const std::set<std::string> &modes) { this->supported_custom_fan_modes_ = modes; }
  void set_action(climate::ClimateAction new_action);

 protected:
  void control(const ClimateCall &call) override;
  ClimateTraits traits() override;
  std::set<ClimateMode> supported_modes_{};
  std::set<ClimateSwingMode> supported_swing_modes_{};
  std::set<ClimatePreset> supported_presets_{};
  std::set<std::string> supported_custom_presets_{};
  std::set<std::string> supported_custom_fan_modes_{};
  Sensor *outdoor_sensor_{nullptr};
  Sensor *humidity_sensor_{nullptr};
  Sensor *power_sensor_{nullptr};
  Sensor *energy_sensor_{nullptr};
  Sensor *t1_sensor_{nullptr};
  Sensor *t2_sensor_{nullptr};
  Sensor *t3_sensor_{nullptr};
  Sensor *t4_sensor_{nullptr};
  Sensor *eev_sensor_{nullptr};
  Sensor *compressor_target_sensor_{nullptr};
  Sensor *compressor_value_sensor_{nullptr};
  Sensor *run_mode_sensor_{nullptr};
  BinarySensor *defrost_sensor_{nullptr};
  Sensor *val1_8_sensor_{nullptr};
  Sensor *val2_12_sensor_{nullptr};
  Sensor *idFTarget_sensor_{nullptr};
  Sensor *idFVal_sensor_{nullptr};
  Sensor *odFVal_sensor_{nullptr};
};

}  // namespace ac
}  // namespace midea
}  // namespace esphome

#endif  // USE_ARDUINO
