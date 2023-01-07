#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

#include <vector>

using namespace esphome;

namespace rego {

class RegoSwitch : public switch_::Switch, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(value != 0.);
  }
  void write_state(bool state) override {
    this->send_data(this->can_poll_id, state);
    this->publish_state(state);
  }
};

}
