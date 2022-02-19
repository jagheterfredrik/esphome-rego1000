#include "esphome.h"

#include "driver/gpio.h"
#include "driver/can.h"

#define GT1_TEMP 0x022C
#define GT2_TEMP 0x0230
#define GT3_TEMP 0x0237
#define GT6_TEMP 0x0250
#define GT8_TEMP 0x026D
#define GT9_TEMP 0x0275
#define GT10_TEMP 0x0212
#define GT11_TEMP 0x0220
#define HEATING_SETPOINT 0x02F4
#define STATS_ENERGY_OUTPUT 0x0714

const uint16_t values_to_poll[] = {GT1_TEMP, GT2_TEMP, GT3_TEMP, GT6_TEMP, GT8_TEMP, GT9_TEMP, GT10_TEMP, GT11_TEMP, HEATING_SETPOINT, STATS_ENERGY_OUTPUT};

static const char* TAG = "RegoReader";

class RegoReader : public Component {
  protected:
    static RegoReader *instance;

    gpio_num_t tx;
    gpio_num_t rx;

    int poll_idx = 0;
    unsigned long lastest_poll = 0;

    RegoReader(gpio_num_t tx, gpio_num_t rx): tx(tx), rx(rx) {}

  public:
    Sensor *heat_carrier_1 = new Sensor();
    Sensor *outdoor = new Sensor();
    Sensor *warm_water = new Sensor();
    Sensor *hot_gas = new Sensor();
    Sensor *heat_fluid_out = new Sensor();
    Sensor *heat_fluid_in = new Sensor();
    Sensor *cold_fluid_in = new Sensor();
    Sensor *cold_fluid_out = new Sensor();

    Sensor *heat_fluid_pump_control = new Sensor();

    Sensor *heating_setpoint = new Sensor();
    Sensor *energy_output = new Sensor();

    BinarySensor *heat_carrier_pump = new BinarySensor();
    BinarySensor *heat_fluid_pump = new BinarySensor();
    BinarySensor *three_way_valve = new BinarySensor();
    BinarySensor *additional_heat = new BinarySensor();
    BinarySensor *compressor = new BinarySensor();
    BinarySensor *cold_fluid_pump = new BinarySensor();

    static RegoReader *getInstance() {
      if (!instance) {
        instance = new RegoReader(GPIO_NUM_23, GPIO_NUM_22);
      }
      return instance;
   }

    void setup() {
      //Initialize configuration structures using macro initializers
      can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(this->tx, this->rx, CAN_MODE_NORMAL);
      g_config.tx_queue_len = 20;
      g_config.rx_queue_len = 20;
      can_timing_config_t t_config = CAN_TIMING_CONFIG_125KBITS();
      can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

      //Install CAN driver
      if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        ESP_LOGD(TAG, "Driver installed\n");
      } else {
        ESP_LOGD(TAG, "Failed to install driver\n");
        return;
      }

      //Start CAN driver
      if (can_start() == ESP_OK) {
        ESP_LOGD(TAG, "Driver started\n");
      } else {
        ESP_LOGD(TAG, "Failed to start driver\n");
        return;
      }
    }

    void query_id(uint16_t idx) {
        can_message_t out_message;
        out_message.flags = CAN_MSG_FLAG_RTR | CAN_MSG_FLAG_EXTD;
        out_message.identifier = (idx << 14) | 0x04003FE0;
        out_message.data_length_code = 0;
        can_transmit(&out_message, pdMS_TO_TICKS(1000));
    }

    void loop() {
      unsigned long now = millis();
      if (now - lastest_poll > 500) {
        this->query_id(values_to_poll[poll_idx++]);
        int number_of_values_to_poll = sizeof(values_to_poll)/sizeof(*values_to_poll);
        poll_idx %= number_of_values_to_poll;
        lastest_poll = now;
      }

      can_message_t message;
      while (can_receive(&message, 0U) == ESP_OK) {
        if (message.flags & CAN_MSG_FLAG_RTR) {
          ESP_LOGD(TAG, " RTR from 0x%08X, DLC %d\r\n", message.identifier,  message.data_length_code);
        }
        else {
          if (message.identifier == 0x00028260) { //Heat carrier pump
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->heat_carrier_pump);
          } else if (message.identifier == 0x0002c260) { //Heat fluid pump
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->heat_fluid_pump);
          } else if (message.identifier == 0x00038260) { //Three-way valve
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->three_way_valve);
          } else if (message.identifier == 0x0003c260) { //Additional heat
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->additional_heat);
          } else if (message.identifier == 0x00048260) { //Compressor
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->compressor);
          } else if (message.identifier == 0x00054260) { //Cold fluid pump
            this->publish_binary(message.identifier, message.data_length_code, message.data, this->cold_fluid_pump);
          } else if (message.identifier == 0x00090260) { //Heat fluid pump control
            this->publish_u8(message.identifier, message.data_length_code, message.data, this->heat_fluid_pump_control);
          } else if (message.identifier == (0x0C003FE0 | (HEATING_SETPOINT << 14))) { //Heating setpoint
           this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->heating_setpoint);
          } else if (message.identifier == (0x0C003FE0 | (GT1_TEMP << 14))) { //Heat carrier 1
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->heat_carrier_1);
          } else if (message.identifier == (0x0C003FE0 | (GT2_TEMP << 14))) { //Outdoor
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->outdoor);
          } else if (message.identifier == (0x0C003FE0 | (GT3_TEMP << 14))) { //Warm water
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->warm_water);
          } else if (message.identifier == (0x0C003FE0 | (GT6_TEMP << 14))) { //Hot gas
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->hot_gas);
          } else if (message.identifier == (0x0C003FE0 | (GT8_TEMP << 14))) { //Heat fluid out
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->heat_fluid_out);
          } else if (message.identifier == (0x0C003FE0 | (GT9_TEMP << 14))) { //Heat fluid in
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->heat_fluid_in);
          } else if (message.identifier == (0x0C003FE0 | (GT10_TEMP << 14))) { //Cold fluid in
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->cold_fluid_in);
          } else if (message.identifier == (0x0C003FE0 | (GT11_TEMP << 14))) { //Cold fluid out
            this->publish_deg_temperature(message.identifier, message.data_length_code, message.data, this->cold_fluid_out);
          } else if (message.identifier == (0x0C003FE0 | (STATS_ENERGY_OUTPUT << 14))) { //Energy output
            this->publish_kwh(message.identifier, message.data_length_code, message.data, this->energy_output);
          } else {
            char buf[64];
            char *bufp = buf;
            for (int i=0; i<message.data_length_code; ++i) {
              bufp += sprintf(bufp, " %.2X", message.data[i]);
            }
            ESP_LOGD(TAG, "%08X (%d):%s", message.identifier, message.data_length_code, buf);
          }
        }
      }
    }

  private:

    void publish_deg_temperature(uint32_t msg_id, uint8_t dlc, uint8_t* data, Sensor *sensor) {
      if (dlc != 2) {
        ESP_LOGE(TAG, "Expected DLC of 2 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      int16_t temperature_val = data[0] << 8 | data[1];
      float temperature = (float)temperature_val/10.f;
      sensor->publish_state(temperature);
    }

    void publish_kwh(uint32_t msg_id, uint8_t dlc, uint8_t* data, Sensor *sensor) {
      if (dlc != 4) {
        ESP_LOGE(TAG, "Expected DLC of 4 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      uint32_t kwh_val = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
      float kwh = (float)kwh_val/100.f;
      sensor->publish_state(kwh);
    }

    void publish_u8(uint32_t msg_id, uint8_t dlc, uint8_t* data, Sensor *sensor) {
      if (dlc != 1) {
        ESP_LOGE(TAG, "Expected DLC of 1 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      sensor->publish_state(data[0]);
    }

    void publish_binary(uint32_t msg_id, uint8_t dlc, uint8_t* data, BinarySensor *sensor) {
      if (dlc != 1) {
        ESP_LOGE(TAG, "Expected DLC of 1 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      sensor->publish_state(!!data[0]);
    }
};

RegoReader *RegoReader::instance = 0;
