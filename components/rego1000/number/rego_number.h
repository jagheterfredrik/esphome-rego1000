#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

#include <vector>

using namespace esphome;

namespace rego {

class RegoNumber : public number::Number, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(value);
  }
  void control(float value) override {
    uint16_t ival = static_cast<uint16_t>(value / this->value_factor);
    this->canbus->send_data(this->can_poll_id, true, false, value_to_can_data(ival));
    this->publish_state(value);
  }
};

}
