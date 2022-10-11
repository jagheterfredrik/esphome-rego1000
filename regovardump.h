#include "esphome.h"

#include "driver/gpio.h"
#include "driver/can.h"

static const char* TAG = "RegoReader";

static const unsigned int MAX_READ_SIZE = 0x100;

struct __attribute__((__packed__)) RegoVariableHeader {
  uint8_t idx[2];
  uint8_t unknown[7];
  uint8_t max_val[4];
  uint8_t min_val[4];
  uint8_t name_length;
} rego_var;

class RegoReader : public Component {
  protected:
    static RegoReader *instance;

    gpio_num_t tx;
    gpio_num_t rx;

    uint8_t *buf;
    uint8_t *bufPtr;

    bool read_hdr = true;
    uint32_t remote_rd_ptr = 0x0;
    uint8_t read_len;

    uint8_t ctr = 0;
    unsigned long cooldown;

    RegoReader(gpio_num_t tx, gpio_num_t rx): tx(tx), rx(rx) {
      buf = (uint8_t*) malloc(MAX_READ_SIZE);
      bufPtr = buf;
    }

    ~RegoReader() {
      if (buf) {
        free(buf);
      }
    }

  public:
    unsigned long state = 0;
    
    static RegoReader *getInstance() {
      if (!instance) {
        instance = new RegoReader(GPIO_NUM_23, GPIO_NUM_22);
      }
      return instance;
    }
    

    void setup() {
      //Initialize configuration structures using macro initializers
      can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(this->tx, this->rx, CAN_MODE_NORMAL);
      g_config.rx_queue_len = 60;
      g_config.tx_queue_len = 60;
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

    void dump_message(can_message_t message) {
      char dumpbuf[64];
      char *dumpbufp = dumpbuf;
      for (int i=0; i<message.data_length_code; ++i) {
        dumpbufp += sprintf(dumpbufp, " %.2X", message.data[i]);
      }
      ESP_LOGD(TAG, "%08X (%d):%s", message.identifier, message.data_length_code, dumpbuf);
    }

    void loop() {
      if(ctr++ == 0) {
        ESP_LOGD(TAG, "current state %d %d", state, millis());
      }

      if (state == 0) { // Set read addr
        if (millis() > 25000) {
          ESP_LOGD(TAG, "STARTING");
          state++;
        }
      }
      if (state == 1) { // Set read addr
        bufPtr = buf;
        read_len = read_hdr ? 18 : rego_var.name_length;

        can_message_t out_message;
        out_message.flags = CAN_MSG_FLAG_EXTD;
        out_message.identifier = 0x01FD3FE0; // read ptr
        out_message.data_length_code = 8;
        out_message.data[0] = 0x00;
        out_message.data[1] = 0x00;
        out_message.data[2] = 0x00;
        out_message.data[3] = read_len;
        out_message.data[4] = (remote_rd_ptr >> 24) & 0xff;
        out_message.data[5] = (remote_rd_ptr >> 16) & 0xff;
        out_message.data[6] = (remote_rd_ptr >> 8) & 0xff;
        out_message.data[7] = remote_rd_ptr & 0xff;
        if (can_transmit(&out_message, pdMS_TO_TICKS(1000)) == ESP_OK) {
          ESP_LOGD(TAG, "Read ptr set QUEUED");
          ++state;
        } else {
          ESP_LOGD(TAG, "Read ptr set FAILED");
        }
      } else if (state == 2) { // Request read
        can_message_t out_message;
        out_message.flags = CAN_MSG_FLAG_RTR | CAN_MSG_FLAG_EXTD;
        out_message.identifier = 0x01FDBFE0; // read cmd
        out_message.data_length_code = 0;
        if (can_transmit(&out_message, pdMS_TO_TICKS(1000)) == ESP_OK) {
          ESP_LOGD(TAG, "Read request QUEUED");
          remote_rd_ptr += read_len;
          ++state;
        } else {
          ESP_LOGD(TAG, "Read request FAILED");
        }
      } else if (state == 3) { // Parse data
        can_message_t message;
        while (can_receive(&message, 0U) == ESP_OK) {
          if (message.identifier == 0x09FDBFE0 || message.identifier == 0x09FDFFE0) {
            // 0x09FDBFE0 - More data available
            // 0x09FDFFE0 - No more data available
            dump_message(message);
            memcpy(bufPtr, message.data, message.data_length_code);
            bufPtr += message.data_length_code;

            ESP_LOGD(TAG, "Got %d now, total %d, want %d", message.data_length_code, read_len, bufPtr - buf);

            if (bufPtr - buf >= read_len) { // Read all data
              if (read_hdr) {
                memcpy(&rego_var, buf, read_len);
                read_hdr = false;
                ESP_LOGD(TAG, "Got header %d, %d, %d, %d", rego_var.idx, rego_var.max_val, rego_var.min_val, rego_var.name_length);
                state = 4;
              } else {
                ESP_LOGI(TAG, "0x%.2X%.2X: %.*s", rego_var.idx[0], rego_var.idx[1], rego_var.name_length, buf);
                read_hdr = true;
                state = 4;
              }
              cooldown = millis();
            }
            if (message.identifier == 0x09FDFFE0) { //No more data
              state = 0xff; //Die
            }
          }
        }
      } else if (state == 4) { // Cool down
        if (millis() - cooldown > 100) { state = 1; }
      }
    }

};

RegoReader *RegoReader::instance = 0;
