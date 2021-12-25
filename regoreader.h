#include "esphome.h"

#include "driver/gpio.h"
#include "driver/can.h"

// Graciously extracted from https://olammi.iki.fi/sw/taloLogger/
static const float temperature_lookup[] = {150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 150.0, 148.2, 146.6, 145.0, 142.4, 140.0, 138.2, 136.6, 135.0, 133.7, 132.4, 131.2, 130.0, 128.7, 127.4, 126.2, 125.0, 123.7, 122.4, 121.2, 120.0, 118.9, 117.9, 116.9, 115.9, 115.0, 114.2, 113.5, 112.8, 112.0, 111.3, 110.7, 110.0, 109.2, 108.5, 107.8, 107.1, 106.4, 105.7, 105.0, 104.3, 103.7, 103.0, 102.4, 101.8, 101.2, 100.6, 100.0, 99.5, 98.9, 98.4, 97.9, 97.4, 96.9, 96.4, 95.9, 95.5, 95.0, 94.5, 94.0, 93.6, 93.1, 92.6, 92.2, 91.7, 91.3, 90.9, 90.4, 90.0, 89.6, 89.2, 88.8, 88.4, 88.0, 87.6, 87.3, 86.9, 86.5, 86.1, 85.8, 85.4, 85.1, 84.7, 84.4, 84.0, 83.7, 83.3, 83.0, 82.7, 82.3, 82.0, 81.7, 81.4, 81.1, 80.8, 80.4, 80.1, 79.8, 79.5, 79.2, 78.9, 78.7, 78.4, 78.1, 77.8, 77.5, 77.2, 77.0, 76.7, 76.4, 76.1, 75.9, 75.6, 75.3, 75.1, 74.8, 74.6, 74.3, 74.1, 73.8, 73.6, 73.3, 73.1, 72.8, 72.6, 72.3, 72.1, 71.9, 71.6, 71.4, 71.1, 70.9, 70.7, 70.5, 70.2, 70.0, 69.8, 69.6, 69.3, 69.1, 68.9, 68.7, 68.5, 68.3, 68.0, 67.8, 67.6, 67.4, 67.2, 67.0, 66.8, 66.6, 66.4, 66.2, 66.0, 65.8, 65.6, 65.4, 65.2, 65.0, 64.8, 64.6, 64.4, 64.2, 64.0, 63.8, 63.7, 63.5, 63.3, 63.1, 62.9, 62.7, 62.5, 62.4, 62.2, 62.0, 61.8, 61.6, 61.5, 61.3, 61.1, 60.9, 60.8, 60.6, 60.4, 60.2, 60.1, 59.9, 59.7, 59.6, 59.4, 59.2, 59.1, 58.9, 58.7, 58.6, 58.4, 58.3, 58.1, 57.9, 57.8, 57.6, 57.5, 57.3, 57.1, 57.0, 56.8, 56.7, 56.5, 56.4, 56.2, 56.1, 55.9, 55.8, 55.6, 55.4, 55.3, 55.2, 55.0, 54.9, 54.7, 54.6, 54.4, 54.3, 54.1, 54.0, 53.8, 53.7, 53.5, 53.4, 53.3, 53.1, 53.0, 52.8, 52.7, 52.6, 52.4, 52.3, 52.1, 52.0, 51.9, 51.7, 51.6, 51.5, 51.3, 51.2, 51.0, 50.9, 50.8, 50.6, 50.5, 50.4, 50.2, 50.1, 50.0, 49.9, 49.7, 49.6, 49.5, 49.3, 49.2, 49.1, 48.9, 48.8, 48.7, 48.6, 48.4, 48.3, 48.2, 48.0, 47.9, 47.8, 47.7, 47.5, 47.4, 47.3, 47.2, 47.0, 46.9, 46.8, 46.7, 46.6, 46.4, 46.3, 46.2, 46.1, 45.9, 45.8, 45.7, 45.6, 45.5, 45.3, 45.2, 45.1, 45.0, 44.9, 44.8, 44.6, 44.5, 44.4, 44.3, 44.2, 44.1, 43.9, 43.8, 43.7, 43.6, 43.5, 43.4, 43.3, 43.1, 43.0, 42.9, 42.8, 42.7, 42.6, 42.5, 42.4, 42.3, 42.1, 42.0, 41.9, 41.8, 41.7, 41.6, 41.5, 41.4, 41.3, 41.2, 41.1, 40.9, 40.8, 40.7, 40.6, 40.5, 40.4, 40.3, 40.2, 40.1, 40.0, 39.9, 39.8, 39.7, 39.5, 39.4, 39.3, 39.2, 39.1, 39.0, 38.9, 38.8, 38.7, 38.6, 38.5, 38.4, 38.3, 38.2, 38.1, 38.0, 37.9, 37.7, 37.6, 37.5, 37.4, 37.3, 37.2, 37.1, 37.0, 36.9, 36.8, 36.7, 36.6, 36.5, 36.4, 36.3, 36.2, 36.1, 36.0, 35.9, 35.8, 35.7, 35.6, 35.5, 35.4, 35.3, 35.2, 35.1, 35.0, 34.9, 34.8, 34.7, 34.6, 34.5, 34.4, 34.3, 34.2, 34.1, 34.0, 33.9, 33.8, 33.7, 33.6, 33.5, 33.4, 33.3, 33.2, 33.1, 33.0, 32.9, 32.9, 32.8, 32.7, 32.6, 32.5, 32.4, 32.3, 32.2, 32.1, 32.0, 31.9, 31.8, 31.7, 31.6, 31.5, 31.4, 31.3, 31.2, 31.2, 31.1, 31.0, 30.9, 30.8, 30.7, 30.6, 30.5, 30.4, 30.3, 30.2, 30.1, 30.0, 29.9, 29.8, 29.8, 29.7, 29.6, 29.5, 29.4, 29.3, 29.2, 29.1, 29.0, 28.9, 28.8, 28.7, 28.6, 28.6, 28.5, 28.4, 28.3, 28.2, 28.1, 28.0, 27.9, 27.8, 27.7, 27.6, 27.5, 27.5, 27.4, 27.3, 27.2, 27.1, 27.0, 26.9, 26.8, 26.7, 26.6, 26.5, 26.5, 26.4, 26.3, 26.2, 26.1, 26.0, 25.9, 25.8, 25.7, 25.6, 25.6, 25.5, 25.4, 25.3, 25.2, 25.1, 25.0, 24.9, 24.8, 24.7, 24.7, 24.6, 24.5, 24.4, 24.3, 24.2, 24.1, 24.0, 23.9, 23.9, 23.8, 23.7, 23.6, 23.5, 23.4, 23.3, 23.2, 23.1, 23.1, 23.0, 22.9, 22.8, 22.7, 22.6, 22.5, 22.4, 22.4, 22.3, 22.2, 22.1, 22.0, 21.9, 21.8, 21.7, 21.6, 21.6, 21.5, 21.4, 21.3, 21.2, 21.1, 21.0, 20.9, 20.9, 20.8, 20.7, 20.6, 20.5, 20.4, 20.3, 20.2, 20.2, 20.1, 20.0, 19.9, 19.8, 19.7, 19.6, 19.5, 19.4, 19.4, 19.3, 19.2, 19.1, 19.0, 18.9, 18.8, 18.7, 18.7, 18.6, 18.5, 18.4, 18.3, 18.2, 18.1, 18.0, 17.9, 17.9, 17.8, 17.7, 17.6, 17.5, 17.4, 17.3, 17.2, 17.1, 17.1, 17.0, 16.9, 16.8, 16.7, 16.6, 16.5, 16.4, 16.4, 16.3, 16.2, 16.1, 16.0, 15.9, 15.8, 15.7, 15.6, 15.6, 15.5, 15.4, 15.3, 15.2, 15.1, 15.0, 14.9, 14.8, 14.8, 14.7, 14.6, 14.5, 14.4, 14.3, 14.2, 14.1, 14.0, 14.0, 13.9, 13.8, 13.7, 13.6, 13.5, 13.4, 13.3, 13.2, 13.1, 13.1, 13.0, 12.9, 12.8, 12.7, 12.6, 12.5, 12.4, 12.3, 12.3, 12.2, 12.1, 12.0, 11.9, 11.8, 11.7, 11.6, 11.5, 11.4, 11.3, 11.3, 11.2, 11.1, 11.0, 10.9, 10.8, 10.7, 10.6, 10.5, 10.4, 10.3, 10.3, 10.2, 10.1, 10.0, 9.9, 9.8, 9.7, 9.6, 9.5, 9.4, 9.3, 9.2, 9.2, 9.1, 9.0, 8.9, 8.8, 8.7, 8.6, 8.5, 8.4, 8.3, 8.2, 8.1, 8.0, 7.9, 7.8, 7.8, 7.7, 7.6, 7.5, 7.4, 7.3, 7.2, 7.1, 7.0, 6.9, 6.8, 6.7, 6.6, 6.5, 6.4, 6.3, 6.2, 6.1, 6.0, 5.9, 5.9, 5.8, 5.7, 5.6, 5.5, 5.4, 5.3, 5.2, 5.1, 5.0, 4.9, 4.8, 4.7, 4.6, 4.5, 4.4, 4.3, 4.2, 4.1, 4.0, 3.9, 3.8, 3.7, 3.6, 3.5, 3.4, 3.3, 3.2, 3.1, 3.0, 2.9, 2.8, 2.7, 2.6, 2.5, 2.4, 2.3, 2.2, 2.1, 2.0, 1.9, 1.8, 1.7, 1.6, 1.5, 1.3, 1.2, 1.1, 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1, -0.0, -0.1, -0.2, -0.3, -0.5, -0.6, -0.7, -0.8, -0.9, -1.0, -1.1, -1.2, -1.3, -1.4, -1.6, -1.7, -1.8, -1.9, -2.0, -2.1, -2.2, -2.3, -2.5, -2.6, -2.7, -2.8, -2.9, -3.0, -3.1, -3.2, -3.4, -3.5, -3.6, -3.7, -3.8, -3.9, -4.1, -4.2, -4.3, -4.4, -4.5, -4.7, -4.8, -4.9, -5.0, -5.1, -5.3, -5.4, -5.5, -5.6, -5.7, -5.9, -6.0, -6.1, -6.2, -6.4, -6.5, -6.6, -6.7, -6.9, -7.0, -7.1, -7.2, -7.4, -7.5, -7.6, -7.8, -7.9, -8.0, -8.2, -8.3, -8.4, -8.5, -8.7, -8.8, -8.9, -9.1, -9.2, -9.4, -9.5, -9.6, -9.8, -9.9, -10.0, -10.2, -10.3, -10.5, -10.6, -10.7, -10.9, -11.0, -11.2, -11.3, -11.5, -11.6, -11.8, -11.9, -12.1, -12.2, -12.3, -12.5, -12.6, -12.8, -13.0, -13.1, -13.3, -13.4, -13.6, -13.7, -13.9, -14.0, -14.2, -14.4, -14.5, -14.7, -14.8, -15.0, -15.2, -15.3, -15.5, -15.7, -15.8, -16.0, -16.2, -16.3, -16.5, -16.7, -16.9, -17.0, -17.2, -17.4, -17.6, -17.8, -17.9, -18.1, -18.3, -18.5, -18.7, -18.9, -19.1, -19.3, -19.4, -19.6, -19.8, -20.0, -20.2, -20.4, -20.6, -20.8, -21.1, -21.3, -21.5, -21.7, -21.9, -22.1, -22.3, -22.5, -22.8, -23.0, -23.2, -23.4, -23.7, -23.9, -24.1, -24.4, -24.6, -24.9, -25.1, -25.4, -25.6, -25.9, -26.1, -26.4, -26.6, -26.9, -27.2, -27.4, -27.7, -28.0, -28.3, -28.6, -28.9, -29.2, -29.5, -29.8, -30.1, -30.4, -30.7, -31.0, -31.4, -31.7, -32.0, -32.4, -32.7, -33.1, -33.5, -33.8, -34.2, -34.6, -35.0, -35.4, -35.7, -36.1, -36.5, -36.9, -37.3, -37.7, -38.2, -38.6, -39.1, -39.5, -40.0, -40.3, -40.7, -41.1, -41.4, -41.8, -42.2, -42.6, -43.1, -43.5, -44.0, -44.5, -45.0, -45.3, -45.5, -45.8, -46.2, -46.5, -46.9, -47.3, -47.7, -48.2, -48.7, -49.3, -50.0, -50.0, -50.0, -50.0, -50.0};

static float temperature_from_adc(uint16_t val) {
  if (val < 0 || val > 1022) {
    return -0;
  }
  return temperature_lookup[val];
}

static const char* TAG = "RegoReader";

class RegoReader : public Component {
  public:
    Sensor *heat_carrier_1 = new Sensor();
    Sensor *outdoor = new Sensor();
    Sensor *warm_water = new Sensor();
    Sensor *heat_carrier_2 = new Sensor();
    Sensor *hot_gas = new Sensor();
    Sensor *heat_fluid_out = new Sensor();
    Sensor *heat_fluid_in = new Sensor();
    Sensor *cold_fluid_in = new Sensor();
    Sensor *cold_fluid_out = new Sensor();

    RegoReader(gpio_num_t tx, gpio_num_t rx): tx(tx), rx(rx) {}

    void setup() {
      //Initialize configuration structures using macro initializers
      can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(this->tx, this->rx, CAN_MODE_NORMAL);
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

    void loop() {
      can_message_t message;

      if (can_receive(&message, 0U) == ESP_OK) {
        if (message.flags & CAN_MSG_FLAG_EXTD) {
          ESP_LOGD(TAG, "New extended frame");
        }
        else {
          ESP_LOGD(TAG, "New standard frame");
        }

        if (message.flags & CAN_MSG_FLAG_RTR) {
          ESP_LOGD(TAG, " RTR from 0x%08X, DLC %d\r\n", message.identifier,  message.data_length_code);
        }
        else {
          ESP_LOGD(TAG, " from 0x%08X, DLC %d, Data ", message.identifier,  message.data_length_code);
          for (int i = 0; i < message.data_length_code; i++) {
            ESP_LOGD(TAG, "0x%02X ", message.data[i]);
          }
          ESP_LOGD(TAG, "\n");

          if (message.identifier == 0x08000270) { //Heat carrier 1
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->heat_carrier_1);
          } else if (message.identifier == 0x08004270) { //Outdoor
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->outdoor);
          } else if (message.identifier == 0x08008270) { //Warm water
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->warm_water);
          } else if (message.identifier == 0x0800c270) { //Heat carrier 2
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->heat_carrier_2);
          } else if (message.identifier == 0x08010270) { //Hot gas
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->hot_gas);
          } else if (message.identifier == 0x08014270) { //Heat fluid out
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->heat_fluid_out);
          } else if (message.identifier == 0x08018270) { //Heat fluid in
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->heat_fluid_in);
          } else if (message.identifier == 0x0801c270) { //Cold fluid in
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->cold_fluid_in);
          } else if (message.identifier == 0x08020270) { //Cold fluid out
            this->publish_temperature(message.identifier, message.data_length_code, message.data, this->cold_fluid_out);
          }
        }
      }
    }

  private:
    gpio_num_t tx;
    gpio_num_t rx;

    void publish_temperature(uint32_t msg_id, uint8_t dlc, uint8_t* data, Sensor *sensor) {
      if (dlc != 2) {
        ESP_LOGE(TAG, "Expected DLC of 2 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      unsigned short temperature_adc_val = data[0] << 8 || data[1];
      float temperature = temperature_from_adc(temperature_adc_val);
      sensor->publish_state(temperature);
    }

    void publish_u8(uint32_t msg_id, uint8_t dlc, uint8_t* data, Sensor *sensor) {
      if (dlc != 1) {
        ESP_LOGE(TAG, "Expected DLC of 1 for 0x%08X, got %d", msg_id, dlc);
        return;
      }
      sensor->publish_state(data[0]);
    }
};
