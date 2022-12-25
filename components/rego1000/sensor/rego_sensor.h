#include "esphome.h"
#include "esphome/components/rego1000/rego.h"

#include <vector>

using namespace esphome;

namespace rego {

class RegoSensor : public sensor::Sensor, public RegoBase {
public:
  virtual void publish(float value) override {
    this->publish_state(value);
  }
};

}
