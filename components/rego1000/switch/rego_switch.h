#pragma once
#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

using namespace esphome;

namespace rego {

class RegoSwitch : public switch_::Switch, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(value != 0.0f);
  }
  void write_state(bool state) override {
    if (this->send_data(this->can_poll_id, state)) {
      this->publish_state(state);
      this->schedule_readback();
    } else {
      // Nothing was sent: revert Home Assistant to the current state
      ESP_LOGW(TAG, "Switch write not sent, keeping current state");
      this->publish_state(this->state);
    }
  }
};

}
