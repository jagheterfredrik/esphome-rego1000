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
    uint32_t ival = value / this->value_factor;
    this->send_data(this->can_poll_id, ival);
    this->publish_state(value);
  }
};

}
