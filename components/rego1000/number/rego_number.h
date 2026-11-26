#pragma once
#include "esphome.h"
#include "esphome/components/rego1000/rego.h"
#include <cmath>

using namespace esphome;

namespace rego {

class RegoNumber : public number::Number, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(value);
  }
  void control(float value) override {
    // Round instead of truncate: 47.0 / 0.1 = 469.99... must become 470
    int32_t ival = (int32_t) std::lround((double) value / this->value_factor);
    if (this->send_data(this->can_poll_id, ival)) {
      this->publish_state(value);
      this->schedule_readback();
    } else {
      // Nothing was sent: revert Home Assistant to the value the pump has
      ESP_LOGW(TAG, "Write of %.2f not sent, keeping current value", value);
      if (this->has_state())
        this->publish_state(this->state);
    }
  }
};

}
