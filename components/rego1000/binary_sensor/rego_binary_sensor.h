#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

#include <vector>

using namespace esphome;

namespace rego {

class RegoBinarySensor : public binary_sensor::BinarySensor, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(static_cast<bool>(value));
  }
};

}
