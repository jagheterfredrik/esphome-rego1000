#include "esphome.h"

#include "driver/gpio.h"
#include "driver/can.h"

static const char* TAG = "RegoReader";

static const unsigned int MAX_READ_SIZE = 0x5000;

typedef struct __attribute__((__packed__)) RegoVariableHeader {
  uint8_t idx[2];
  uint8_t unknown[7];
  uint8_t max_val[4];
  uint8_t min_val[4];
  uint8_t name_length;
} rego_hdr;

class RegoReader : public Component {
  protected:
    static RegoReader *instance;

    gpio_num_t tx;
    gpio_num_t rx;

    uint8_t *buf;
    uint8_t *buf_ptr;
    uint8_t leftover = 0;

    bool read_hdr = true;
    uint32_t remote_rd_ptr = 0x0;

    uint8_t ctr = 0;
    unsigned long cooldown;

    RegoReader(gpio_num_t tx, gpio_num_t rx): tx(tx), rx(rx) {
      buf = (uint8_t*) malloc(MAX_READ_SIZE);
      buf_ptr = buf;
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
        instance = new RegoReader(GPIO_NUM_5, GPIO_NUM_35);
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
        buf_ptr = &buf[leftover];

        can_message_t out_message;
        out_message.flags = CAN_MSG_FLAG_EXTD;
        out_message.identifier = 0x01FD3FE0; // read ptr
        out_message.data_length_code = 8;
        out_message.data[0] = 0x00;
        out_message.data[1] = 0x00;
        out_message.data[2] = 0x4E;
        out_message.data[3] = 0x20;
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
          remote_rd_ptr += 0x4E20;
          state = 0xff;
        } else {
          ESP_LOGD(TAG, "Read request FAILED");
        }
      } else if (state == 3 || state == 4) { // Parse data
        uint8_t *parse_ptr = buf;
        while(1) {
          if (buf_ptr - parse_ptr < sizeof(rego_hdr)) break;
          rego_hdr *hdr = (rego_hdr *)parse_ptr;
          if (buf_ptr - parse_ptr < sizeof(rego_hdr) + hdr->name_length) break;
          parse_ptr += sizeof(rego_hdr);
          
          ESP_LOGI(TAG, "0x%.2X%.2X: %.*s", hdr->idx[0], hdr->idx[1], hdr->name_length, parse_ptr);

          parse_ptr += hdr->name_length;
        }

        leftover = buf_ptr - parse_ptr;
        if (leftover > 0) {
          memcpy(buf, parse_ptr, leftover);
        }
        if (state == 3)
          state = 1;
        else
          state = 0xff;
      }

      can_message_t message;
      while (can_receive(&message, 0U) == ESP_OK) {
        if (message.identifier == 0x09FDBFE0 || message.identifier == 0x09FDFFE0) {
          // 0x09FDBFE0 - More data available
          // 0x09FDFFE0 - No more data available
          
          memcpy(buf_ptr, message.data, message.data_length_code);
          buf_ptr += message.data_length_code;

          if ((buf_ptr - buf) % 0x500 == 0)
            ESP_LOGD(TAG, "read %d", buf_ptr - buf);

          if (buf_ptr - buf >= 0x4e20 + leftover) { // Read all data
            state = 3;
          }
          if (message.identifier == 0x09FDFFE0) { //No more data
            state = 4;
            ESP_LOGD(TAG, "Got end of data flag");
          }
        }
      }
    }

};

RegoReader *RegoReader::instance = 0;
