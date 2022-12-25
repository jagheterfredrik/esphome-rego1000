#pragma once
#include "esphome.h"

#include <vector>

using namespace esphome;

namespace rego {
static const char* TAG = "Rego";

class CanCallbackInterface
{
  public:
  virtual void data_recv(std::vector<uint8_t>, uint32_t) = 0;
};

class CanbusTriggerProxy : public canbus::CanbusTrigger, Automation<std::vector<uint8_t>, uint32_t, bool>, Action<std::vector<uint8_t>, uint32_t, bool> {
    CanCallbackInterface *callback;
public:
    CanbusTriggerProxy(canbus::Canbus *canbus, uint32_t can_id, CanCallbackInterface *callback) : CanbusTrigger(canbus, can_id, 0x1fffffff, true), Automation(this), callback(callback) {
        this->add_action(this);
    }
    virtual void play(std::vector<uint8_t> data, uint32_t can_id, bool rtr) override {
        this->callback->data_recv(data, can_id);
    }
};

class RegoBase : public Component, public CanCallbackInterface {
public:
  virtual void setup() override {
    this->setup_listening();
    if (this->is_polling) {
      this->setup_polling();
    }
  }
  /* CAN listening */
  void setup_listening() {
    this->can_trigger = new CanbusTriggerProxy(this->canbus, this->can_recv_id, this);
    this->can_trigger->setup();
  }

  int parse_data(std::vector<uint8_t> data) {
    if (data.size() == 1) {
        return (int8_t)data[0];
    } else if (data.size() == 2) {
        return (int16_t)(data[0] << 8 | data[1]);
    } else if (data.size() == 4) {
        return (int32_t)(data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);
    } else {
        ESP_LOGE(TAG, "Received DLC %d is invalid. Has to be 1, 2 or 4", data.size());
    }
    return 0;
  }

  virtual void data_recv(std::vector<uint8_t> data, uint32_t can_id) {
    int value = this->parse_data(data);
    this->publish(value * this->value_factor);
  }
  virtual void publish(float value)=0;

  /* CAN polling */
  void setup_polling() {
    this->set_interval("rego_poll", this->poll_interval, [this]() { this->rego_poll(); });
  }
  void rego_poll() {
    this->canbus->send_data(this->can_poll_id, true, true, std::vector<uint8_t>());
  }

  /* Config */
  void set_canbus(canbus::Canbus *canbus) {
    this->canbus = canbus;
  }

  void set_rego_variable(uint16_t rego_variable) {
    this->can_recv_id = 0x0C003FE0 | (rego_variable << 14);
    this->can_poll_id = (rego_variable << 14) | 0x04003FE0;
    this->is_polling = true;
  }

  void set_rego_listen_can_id(uint32_t can_id) {
    this->can_recv_id = can_id;
    this->can_poll_id = 0;
    this->is_polling = false;
  }

  void set_value_factor(float value_factor) {
    this->value_factor = value_factor;
  }

  void set_poll_interval(uint32_t poll_interval) {
    this->poll_interval = poll_interval;
  }

  /* Helpers */
  std::vector<uint8_t> value_to_can_data(uint16_t ival) {
    return std::vector<uint8_t>({ static_cast<uint8_t>((ival >> 8) & 0xff), static_cast<uint8_t>(ival & 0xff) });
  }

protected:
  uint32_t can_recv_id;
  uint32_t can_poll_id;
  uint32_t poll_interval;
  bool is_polling = false;
  float value_factor = 1;
  canbus::Canbus *canbus;
  canbus::CanbusTrigger *can_trigger;
};

}
