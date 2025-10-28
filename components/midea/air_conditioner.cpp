#include "Appliance/AirConditioner/AirConditioner.h"
#include "Helpers/Timer.h"
#include "Helpers/Log.h"

namespace dudanov {
namespace midea {
namespace ac {

static const char *TAG = "AirConditioner";

void AirConditioner::m_runSeq() {

  if (this->m_seq == 0)
    this->m_getStatus();
  else if(this->m_seq == 1){
    this->m_getDiag1();
  }
  else if(this->m_seq == 2){
    this->m_getStatus();
  }
  else if(this->m_seq == 3){
    this->m_getDiag2();
  }
  else if(this->m_seq == 4){
    this->m_getStatus();
  }
  else if(this->m_seq == 5){
    this->m_getDiag3();
  }

  this->m_seq++;
  if(this->m_seq>5){this->m_seq=0;}
}

void AirConditioner::m_setup() {
  if (this->m_autoconfStatus != AUTOCONF_DISABLED)
    this->m_getCapabilities();
  this->m_timerManager.registerTimer(this->m_powerUsageTimer);
  this->m_powerUsageTimer.setCallback([this](Timer *timer) {
    timer->reset();
    this->m_getPowerUsage();
  });
  this->m_powerUsageTimer.start(30000);
}

static bool checkConstraints(const Mode &mode, const Preset &preset) {
  if (mode == Mode::MODE_OFF)
    return preset == Preset::PRESET_NONE;
  switch (preset) {
    case Preset::PRESET_NONE:
      return true;
    case Preset::PRESET_ECO:
      return mode == Mode::MODE_COOL;
    case Preset::PRESET_TURBO:
      return mode == Mode::MODE_COOL || mode == Mode::MODE_HEAT;
    case Preset::PRESET_SLEEP:
      return mode != Mode::MODE_DRY && mode != Mode::MODE_FAN_ONLY;
    case Preset::PRESET_FREEZE_PROTECTION:
      return mode == Mode::MODE_HEAT;
    default:
      return false;
  }
}

void AirConditioner::control(const Control &control) {
  if (this->m_sendControl)
    return;
  StatusData status = this->m_status;
  Mode mode = this->m_mode;
  Preset preset = this->m_preset;
  bool hasUpdate = false;
  bool isModeChanged = false;
  if (control.mode.hasUpdate(mode)) {
    hasUpdate = true;
    isModeChanged = true;
    mode = control.mode.value();
    if (this->m_mode == Mode::MODE_OFF)
      preset = this->m_lastPreset;
    else if (!checkConstraints(mode, preset))
      preset = Preset::PRESET_NONE;
  }
  if (control.preset.hasUpdate(preset) && checkConstraints(mode, control.preset.value())) {
    hasUpdate = true;
    preset = control.preset.value();
  }
  if (mode != Mode::MODE_OFF) {
    if (mode == Mode::MODE_AUTO || preset != Preset::PRESET_NONE) {
      if (this->m_fanMode != FanMode::FAN_AUTO) {
        hasUpdate = true;
        status.setFanMode(FanMode::FAN_AUTO);
      }
    } else if (control.fanMode.hasUpdate(this->m_fanMode)) {
      hasUpdate = true;
      status.setFanMode(control.fanMode.value());
    }
    if (control.swingMode.hasUpdate(this->m_swingMode)) {
      hasUpdate = true;
      status.setSwingMode(control.swingMode.value());
    }
  }
  if (control.targetTemp.hasUpdate(this->m_targetTemp)) {
    hasUpdate = true;
    status.setTargetTemp(control.targetTemp.value());
  }
  if (hasUpdate) {
    this->m_sendControl = true;
    status.setMode(mode);
    status.setPreset(preset);
    status.setBeeper(this->m_beeper);
    status.appendCRC();
    if (isModeChanged && preset != Preset::PRESET_NONE && preset != Preset::PRESET_SLEEP) {
      // Last command with preset
      this->m_setStatus(status);
      status.setPreset(Preset::PRESET_NONE);
      status.setBeeper(false);
      status.updateCRC();
      // First command without preset
      this->m_queueRequestPriority(FrameType::DEVICE_CONTROL, std::move(status),
        // onData
        std::bind(&AirConditioner::m_readStatus, this, std::placeholders::_1)
      );
    } else {
      this->m_setStatus(std::move(status));
    }
  }
}

void AirConditioner::m_setStatus(StatusData status) {
  LOG_D(TAG, "Enqueuing a priority SET_STATUS(0x40) request...");
  this->m_queueRequestPriority(FrameType::DEVICE_CONTROL, std::move(status),
    // onData
    std::bind(&AirConditioner::m_readStatus, this, std::placeholders::_1),
    // onSuccess
    [this]() {
      this->m_sendControl = false;
    },
    // onError
    [this]() {
      LOG_W(TAG, "SET_STATUS(0x40) request failed...");
      this->m_sendControl = false;
    }
  );
}

void AirConditioner::setPowerState(bool state) {
  if (state != this->getPowerState()) {
    Control control;
    control.mode = state ? this->m_status.getRawMode() : Mode::MODE_OFF;
    this->control(control);
  }
}

void AirConditioner::m_getPowerUsage() {
  QueryPowerData data{};
  LOG_D(TAG, "Enqueuing a GET_POWERUSAGE(0x41 00 00 04) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    [this](FrameData data) -> ResponseStatus {
      const auto status = data.to<StatusData>();
      if (!status.hasValues())
        return ResponseStatus::RESPONSE_WRONG;
      LOG_D(TAG, "Receiving Power/Energy Values");
//      if (status.hasValuesPower()) {
        if (this->m_energyUsage != status.getEnergyUsage()) {
          this->m_energyUsage = status.getEnergyUsage();
          this->sendUpdate();
          LOG_D(TAG, "Updating EnergyUsage");
        }
        if (this->m_powerUsage != status.getPowerUsage()) {
          this->m_powerUsage = status.getPowerUsage();
          this->sendUpdate();
          LOG_D(TAG, "Updating PowerUsage");
        }
//      }
      return ResponseStatus::RESPONSE_OK;
    }
  );
}

void AirConditioner::m_getCapabilities() {
  GetCapabilitiesData data{};
  this->m_autoconfStatus = AUTOCONF_PROGRESS;
  LOG_D(TAG, "Enqueuing a priority GET_CAPABILITIES(0xB5) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    [this](FrameData data) -> ResponseStatus {
      if (!data.hasID(0xB5))
        return ResponseStatus::RESPONSE_WRONG;
      if (this->m_capabilities.read(data)) {
        GetCapabilitiesSecondData data{};
        this->m_sendFrame(FrameType::DEVICE_QUERY, data);
        return ResponseStatus::RESPONSE_PARTIAL;
      }
      return ResponseStatus::RESPONSE_OK;
    },
    // onSuccess
    [this]() {
      this->m_autoconfStatus = AUTOCONF_OK;
    },
    // onError
    [this]() {
      LOG_W(TAG, "Failed to get 0xB5 capabilities report.");
      this->m_autoconfStatus = AUTOCONF_ERROR;
    }
  );
}

void AirConditioner::m_getStatus() {
  QueryStateData data{};
  LOG_D(TAG, "Enqueuing a GET_STATUS(41 00 00 FF) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    std::bind(&AirConditioner::m_readStatus, this, std::placeholders::_1)
  );
}

void AirConditioner::m_displayToggle() {
  DisplayToggleData data{};
  LOG_D(TAG, "Enqueuing a priority TOGGLE_LIGHT(0x41) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    std::bind(&AirConditioner::m_readStatus, this, std::placeholders::_1)
  );
}

template<typename T>
void setProperty(T &property, const T &value, bool &update) {
  if (property != value) {
    property = value;
    update = true;
  }
}

ResponseStatus AirConditioner::m_readStatus(FrameData data) {
  if (!data.hasStatus())
    return ResponseStatus::RESPONSE_WRONG;
  LOG_D(TAG, "New status data received. Parsing...");
  bool hasUpdate = false;
  const StatusData newStatus = data.to<StatusData>();
  this->m_status.copyStatus(newStatus);
  if (this->m_mode != newStatus.getMode()) {
    hasUpdate = true;
    this->m_mode = newStatus.getMode();
    if (newStatus.getMode() == Mode::MODE_OFF)
      this->m_lastPreset = this->m_preset;
  }
  setProperty(this->m_preset, newStatus.getPreset(), hasUpdate);
  setProperty(this->m_fanMode, newStatus.getFanMode(), hasUpdate);
  setProperty(this->m_swingMode, newStatus.getSwingMode(), hasUpdate);
  setProperty(this->m_targetTemp, newStatus.getTargetTemp(), hasUpdate);
  setProperty(this->m_indoorTemp, newStatus.getIndoorTemp(), hasUpdate);
  setProperty(this->m_outdoorTemp, newStatus.getOutdoorTemp(), hasUpdate);
  setProperty(this->m_indoorHumidity, newStatus.getHumiditySetpoint(), hasUpdate);
  if (hasUpdate)
    this->sendUpdate();
  return ResponseStatus::RESPONSE_OK;
}

void AirConditioner::m_getDiag1() {
  QueryDiagData1 data{};
  LOG_D(TAG, "Enqueuing a GET_STATUS(41 00 00 01) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    std::bind(&AirConditioner::m_readDiag1, this, std::placeholders::_1)
  );
}
void AirConditioner::m_getDiag2() {
  QueryDiagData2 data{};
  LOG_D(TAG, "Enqueuing a GET_STATUS(41 00 00 02) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    std::bind(&AirConditioner::m_readDiag2, this, std::placeholders::_1)
  );
}
void AirConditioner::m_getDiag3() {
  QueryDiagData3 data{};
  LOG_D(TAG, "Enqueuing a GET_STATUS(41 00 00 03) request...");
  this->m_queueRequest(FrameType::DEVICE_QUERY, std::move(data),
    // onData
    std::bind(&AirConditioner::m_readDiag3, this, std::placeholders::_1)
  );
}
// --------------------------------------------------------------------------------
ResponseStatus AirConditioner::m_readDiag1(FrameData data) {
  if (! (data.hasValues() && (data.subType()==1)) )
    return ResponseStatus::RESPONSE_WRONG;

  LOG_D(TAG, "New diag1 data received. Parsing...");
  bool hasUpdate = false;

  const DiagData newDiag = data.to<DiagData>();
  this->m_diag.copyDiag(newDiag);
  setProperty(this->m_compressorSpeed, newDiag.getCompressorSpeed(), hasUpdate);
  setProperty(this->m_t1Temp, newDiag.getT1Temp(), hasUpdate);
  setProperty(this->m_t2Temp, newDiag.getT2Temp(), hasUpdate);
  setProperty(this->m_t3Temp, newDiag.getT3Temp(), hasUpdate);
  setProperty(this->m_t4Temp, newDiag.getT4Temp(), hasUpdate);
  setProperty(this->m_eev, newDiag.getEEV(), hasUpdate);
  setProperty(this->m_runMode, newDiag.getRunMode(), hasUpdate);
  setProperty(this->m_val1_8, newDiag.getVal1_8(), hasUpdate);

  if (hasUpdate)
    this->sendUpdate();
  return ResponseStatus::RESPONSE_OK;
}
// --------------------------------------------------------------------------------
ResponseStatus AirConditioner::m_readDiag2(FrameData data) {
  if (! ( data.hasValues() && (data.subType()==2) ))
    return ResponseStatus::RESPONSE_WRONG;

  LOG_D(TAG, "New diag2 data received. Parsing...");
  bool hasUpdate = false;

  const DiagData newDiag = data.to<DiagData>();
  this->m_diag.copyDiag(newDiag);
  setProperty(this->m_idFTarget, newDiag.getIdFTarget(), hasUpdate);
  setProperty(this->m_idFVal, newDiag.getIdFVal(), hasUpdate);
  setProperty(this->m_defrost, newDiag.getDefrost(), hasUpdate);
  setProperty(this->m_val2_12, newDiag.getVal2_12(), hasUpdate);

  if (hasUpdate)
    this->sendUpdate();
  return ResponseStatus::RESPONSE_OK;
}
// --------------------------------------------------------------------------------
ResponseStatus AirConditioner::m_readDiag3(FrameData data) {
  if (! ( data.hasValues() && (data.subType()==3) ))
    return ResponseStatus::RESPONSE_WRONG;

  LOG_D(TAG, "New diag3 data received. Parsing...");
  bool hasUpdate = false;

  const DiagData newDiag = data.to<DiagData>();
  this->m_diag.copyDiag(newDiag);
  setProperty(this->m_odFVal, newDiag.getOdFVal(), hasUpdate);
  setProperty(this->m_compressorTarget, newDiag.getCompressorTarget(), hasUpdate);

  if (hasUpdate)
    this->sendUpdate();
  return ResponseStatus::RESPONSE_OK;
}

}  // namespace ac
}  // namespace midea
}  // namespace dudanov
