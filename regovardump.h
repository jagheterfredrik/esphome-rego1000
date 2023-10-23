#include "esphome.h"

static const char* TAG = "RegoReader";

static const unsigned int MAX_READ_SIZE = 0x5000;

typedef struct __attribute__((__packed__)) RegoVariableHeader {
  uint8_t idx[2];
  uint8_t unknown[7];
  uint8_t max_val[4];
  uint8_t min_val[4];
  uint8_t name_length;
} rego_hdr;

class CanCallbackInterface
{
  public:
  virtual void data_recv(std::vector<uint8_t>, uint32_t) = 0;
};

class CanbusTriggerProxy : public canbus::CanbusTrigger, Automation<std::vector<uint8_t>, uint32_t, bool>, Action<std::vector<uint8_t>, uint32_t, bool> {
    CanCallbackInterface *callback;
public:
    CanbusTriggerProxy(canbus::Canbus *canbus, CanCallbackInterface *callback) : CanbusTrigger(canbus, 0, 0, true), Automation(this), callback(callback) {
        this->add_actions({this});
    }
    virtual void play(std::vector<uint8_t> data, uint32_t can_id, bool rtr) override {
        this->callback->data_recv(data, can_id);
    }
};

class RegoReader : public Component, public CanCallbackInterface {
  protected:
    static RegoReader *instance;

    canbus::Canbus *canbus;
    canbus::CanbusTrigger *can_trigger;

    uint8_t *buf;
    uint8_t *buf_ptr;
    uint8_t leftover = 0;

    bool read_hdr = true;
    uint32_t remote_rd_ptr = 0x0;

    uint8_t ctr = 0;
    unsigned long cooldown;

    RegoReader(canbus::Canbus *canbus): canbus(canbus) {
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
    
    static RegoReader *getInstance(canbus::Canbus *canbus) {
      if (!instance) {
        instance = new RegoReader(canbus);
      }
      return instance;
    }

    void setup() {
      this->can_trigger = new CanbusTriggerProxy(this->canbus, this);
      this->can_trigger->setup();
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
        std::vector<uint8_t> can_data = std::vector<uint8_t>({
          (uint8_t)0x00,
          (uint8_t)0x00,
          (uint8_t)0x08,
          (uint8_t)0x00,
          (uint8_t) ((remote_rd_ptr >> 24) & 0xff),
          (uint8_t) ((remote_rd_ptr >> 16) & 0xff),
          (uint8_t) ((remote_rd_ptr >> 8) & 0xff),
          (uint8_t) (remote_rd_ptr & 0xff)
        });
        this->canbus->send_data(0x01FD3FE0, true, false, can_data);
        ++state;
      } else if (state == 2) { // Request read
        this->canbus->send_data(0x01FDBFE0, true, true, std::vector<uint8_t>());
        remote_rd_ptr += 0x800;
        state = 0xff;
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
  }

  virtual void data_recv(std::vector<uint8_t> data, uint32_t can_id) {
    if (can_id == 0x09FDBFE0 || can_id == 0x09FDFFE0) {
      // 0x09FDBFE0 - More data available
      // 0x09FDFFE0 - No more data available
      
      memcpy(buf_ptr, data.data(), data.size());
      buf_ptr += data.size();

      if ((buf_ptr - buf) % 0x500 == 0)
        ESP_LOGD(TAG, "read %d", buf_ptr - buf);

      if (buf_ptr - buf >= 0x800 + leftover) { // Read all data
        state = 3;
      }
      if (can_id == 0x09FDFFE0) { //No more data
        state = 4;
        ESP_LOGD(TAG, "Got end of data flag");
      }
    }
  }

};

RegoReader *RegoReader::instance = 0;
