#pragma once
#include "esphome.h"
#include "esphome/components/rego1000/rego.h"
#include <cmath>

#define INDOOR_THERMOSTAT_TEMP_CAN_ID 0x10000060
#define INDOOR_THERMOSTAT_DIAL_CAN_ID 0x10004060
#define INDOOR_THERMOSTAT_DIAL_MIDPOINT 0x200

using namespace esphome;
using namespace esphome::climate;

namespace rego {

class RegoClimate : public Climate, public RegoBase {
public:
  RegoClimate() {
    this->preset = CLIMATE_PRESET_HOME;
    this->mode = CLIMATE_MODE_HEAT;
    this->action = CLIMATE_ACTION_HEATING;
  }

  void setup() override {
    RegoBase::setup();
    // Poll the setpoint right after boot. The indoor temperature cannot be sent
    // until a first reply has been received (it determines the data length), and
    // with a long poll_interval the pump would otherwise raise a room sensor alarm.
    this->set_timeout(SCHED_ID_INITIAL_POLL, 1000, [this]() { this->rego_poll(); });
  }

  ClimateTraits traits() override {
    auto traits = climate::ClimateTraits();
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
    traits.set_supported_modes({climate::CLIMATE_MODE_HEAT});
    traits.set_visual_min_temperature(12.0);
    traits.set_visual_max_temperature(25.0);
    traits.set_visual_temperature_step(0.1);
    return traits;
  }

  virtual void publish(float value) override {
    float target = value / 10.0f;
    if (std::isnan(this->target_temperature) || std::fabs(this->target_temperature - target) >= 0.05f) {
      this->target_temperature = target;
      this->publish_state();
    }
  }

  void set_indoor_sensor(sensor::Sensor *indoor_sensor) {
    this->indoor_sensor = indoor_sensor;
    this->set_interval(SCHED_ID_INDOOR_TEMP, 5000, [this]() { this->update_indoor_temperature(); });
  }

  void update_indoor_temperature() {
    if (this->indoor_sensor == nullptr || !this->indoor_sensor->has_state())
      return;

    float temp = this->indoor_sensor->state;

    // NaN (Home Assistant entity "unavailable" or a timeout in YAML): send nothing.
    // The pump raises its room sensor alarm and falls back to the heating curve.
    if (std::isnan(temp))
      return;

    // Publish to Home Assistant only when the value has changed
    if (std::isnan(this->current_temperature) || std::fabs(this->current_temperature - temp) >= 0.05f) {
      this->current_temperature = temp;
      this->publish_state();
    }

    // Always send to the pump, every 5 seconds
    int32_t indoor_temp = std::lround(temp * 10.0f);
    this->send_data(INDOOR_THERMOSTAT_DIAL_CAN_ID, INDOOR_THERMOSTAT_DIAL_MIDPOINT);
    this->send_data(INDOOR_THERMOSTAT_TEMP_CAN_ID, indoor_temp);
  }

  void control(const ClimateCall &call) override {
    if (!call.get_target_temperature().has_value())
      return;

    float requested = *call.get_target_temperature();
    int32_t indoor_setpoint = std::lround(requested * 10.0f);
    if (this->send_data(this->can_poll_id, indoor_setpoint)) {
      this->target_temperature = requested;
      this->schedule_readback();
    } else {
      ESP_LOGW(TAG, "Setpoint write not sent, keeping current target");
    }
    // Publish in both cases so Home Assistant shows the current setpoint
    this->publish_state();
  }

protected:
  sensor::Sensor *indoor_sensor{nullptr};
};

}  // namespace rego
