#pragma once
#include "esphome.h"
#include <vector>

using namespace esphome;

namespace rego {
static const char* TAG = "Rego";

// Delay before reading a value back after a write (ms)
static const uint32_t READBACK_DELAY_MS = 500;

// Numeric scheduler IDs (ESPHome 2026.11 recommends IDs instead of string names)
static constexpr uint32_t SCHED_ID_POLL = 1;
static constexpr uint32_t SCHED_ID_READBACK = 2;
static constexpr uint32_t SCHED_ID_INDOOR_TEMP = 3;
static constexpr uint32_t SCHED_ID_INITIAL_POLL = 4;

class CanCallbackInterface
{
  public:
  virtual void data_recv(const std::vector<uint8_t>& data, uint32_t can_id) = 0;
};

class CanCallbackAction : public Action<std::vector<uint8_t>, uint32_t, bool> {
    CanCallbackInterface *callback;
public:
    CanCallbackAction(CanCallbackInterface *callback) : callback(callback) {}

    void play(const std::vector<uint8_t>& data, const uint32_t& can_id, const bool& rtr) override {
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

  void setup_listening() {
    this->can_trigger = new canbus::CanbusTrigger(this->canbus, this->can_recv_id, 0x1fffffff, true);
    auto *automation = new Automation<std::vector<uint8_t>, uint32_t, bool>(this->can_trigger);
    automation->add_actions({new CanCallbackAction(this)});
    this->can_trigger->setup();
  }

  int parse_data(const std::vector<uint8_t>& data) {
    if (data.size() == 1) {
        return (int8_t)data[0];
    } else if (data.size() == 2) {
        return (int16_t)(data[0] << 8 | data[1]);
    } else if (data.size() == 4) {
        return (int32_t)(data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);
    } else {
        ESP_LOGE(TAG, "Received DLC %zu is invalid. Has to be 1, 2 or 4", data.size());
    }
    return 0;
  }

  void data_recv(const std::vector<uint8_t>& data, uint32_t can_id) override {
    this->inferred_data_len = data.size();
    int value = this->parse_data(data);
    // Multiply in double precision so large counters (e.g. energy) are only
    // rounded once, when converted to float in publish().
    this->publish((float)((double)value * this->value_factor));
  }

  virtual void publish(float value)=0;

  void setup_polling() {
    this->set_interval(SCHED_ID_POLL, this->poll_interval, [this]() { this->rego_poll(); });
  }

  void rego_poll() {
    this->canbus->send_data(this->can_poll_id, true, true, {});
  }

  // Read the value back shortly after a write so Home Assistant shows what the
  // heat pump actually has (the pump may reject or overwrite the value).
  void schedule_readback() {
    if (!this->is_polling)
      return;
    this->set_timeout(SCHED_ID_READBACK, READBACK_DELAY_MS, [this]() { this->rego_poll(); });
  }

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

  void set_value_factor(double value_factor) {
    this->value_factor = value_factor;
  }

  void set_poll_interval(uint32_t poll_interval) {
    this->poll_interval = poll_interval;
  }

  // Returns true if the CAN driver accepted the frame, false if nothing was sent
  bool send_data(uint32_t can_id, int32_t value) {
    std::vector<uint8_t> payload;
    if (this->inferred_data_len == 0) {
      ESP_LOGW(TAG, "Not sending to CAN ID 0x%08X before a value has been received", (unsigned) can_id);
      return false;
    } else if (this->inferred_data_len == 1) {
      payload = { (uint8_t)value };
    } else if (this->inferred_data_len == 2) {
      payload = { (uint8_t)((value >> 8) & 0xff), (uint8_t)(value & 0xff) };
    } else if (this->inferred_data_len == 4) {
      payload = { (uint8_t)((value >> 24) & 0xff), (uint8_t)((value >> 16) & 0xff), (uint8_t)((value >> 8) & 0xff), (uint8_t)(value & 0xff) };
    } else {
      ESP_LOGE(TAG, "Tried to send data with invalid inferred length %d", this->inferred_data_len);
      return false;
    }
    canbus::Error err = this->canbus->send_data(can_id, true, false, payload);
    if (err != canbus::ERROR_OK) {
      ESP_LOGW(TAG, "CAN send to 0x%08X failed (error %d)", (unsigned) can_id, (int) err);
      return false;
    }
    return true;
  }

protected:
  uint32_t can_recv_id;
  uint32_t can_poll_id;
  uint32_t poll_interval;
  uint8_t inferred_data_len = 0;
  bool is_polling = false;
  double value_factor = 1;
  canbus::Canbus *canbus;
  canbus::CanbusTrigger *can_trigger;
};
}
