#include "sunspec_server.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include <cstring>
#include <cmath>

namespace esphome {
namespace sunspec_modbus_server {

static const char *const TAG = "sunspec_modbus_server";

// Modbus function codes
static const uint8_t FC_READ_HOLDING_REGISTERS = 0x03;
static const uint8_t FC_READ_INPUT_REGISTERS = 0x04;
static const uint8_t FC_WRITE_SINGLE_REGISTER = 0x06;
static const uint8_t FC_WRITE_MULTIPLE_REGISTERS = 0x10;

// Modbus exception codes
static const uint8_t EX_ILLEGAL_FUNCTION = 0x01;
static const uint8_t EX_ILLEGAL_DATA_ADDRESS = 0x02;
static const uint8_t EX_ILLEGAL_DATA_VALUE = 0x03;
static const uint8_t EX_SERVER_DEVICE_FAILURE = 0x04;

// Modbus TCP header size (MBAP)
static const size_t MBAP_HEADER_SIZE = 7;
static const size_t MIN_REQUEST_SIZE = 12;  // MBAP + Unit ID + FC + Start Addr + Quantity

// Clamp a float to a valid uint16_t register value.
// Returns 0 for NaN, negative, or subnormal; 65535 for Inf or overflow.
static inline uint16_t safe_u16(float v) {
  if (!std::isfinite(v) || v < 0.0f) return 0;
  if (v > 65535.0f) return 65535;
  return static_cast<uint16_t>(v);
}

SunSpecModbusServer::SunSpecModbusServer() {
  memset(this->registers_, 0, sizeof(this->registers_));
}

void SunSpecModbusServer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SunSpec Modbus TCP Server...");

  // Initialize SunSpec registers with static data
  this->init_registers_();

  // Initialize timing
  this->last_update_ = millis();

  // Start TCP server
  this->start_server_();
}

void SunSpecModbusServer::loop() {
  // Update values from source sensors
  uint32_t now = millis();
  if (now - this->last_update_ >= this->update_interval_) {
    this->update_from_sources_();
    this->update_registers_();
    this->publish_sensors_();
    this->last_update_ = now;
  }

  // Check revert timer: restore full power if Victron stops sending commands
  if (this->revert_active_ && (now - this->revert_deadline_) < 0x80000000U) {
    this->revert_active_ = false;
    this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLim_Ena] = 0;
    this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct] = 100;
    if (this->power_limit_number_ != nullptr) {
      ESP_LOGW(TAG, "Revert timer expired — restoring full power (100%%)");
      auto call = this->power_limit_number_->make_call();
      call.set_value(100.0f);
      call.perform();
    }
  }

  // Handle Modbus TCP clients
  this->handle_client_();
}

void SunSpecModbusServer::dump_config() {
  ESP_LOGCONFIG(TAG, "SunSpec Modbus TCP Server:");
  ESP_LOGCONFIG(TAG, "  Port: %u", this->port_);
  ESP_LOGCONFIG(TAG, "  Unit ID: %u", this->unit_id_);
  ESP_LOGCONFIG(TAG, "  Manufacturer: %s", this->manufacturer_.c_str());
  ESP_LOGCONFIG(TAG, "  Model: %s", this->model_.c_str());
  ESP_LOGCONFIG(TAG, "  Serial: %s", this->serial_.c_str());
  ESP_LOGCONFIG(TAG, "  Update Interval: %u ms", this->update_interval_);
}

void SunSpecModbusServer::start_server_() {
  this->server_ = new WiFiServer(this->port_);
  if (!this->server_) {
    ESP_LOGE(TAG, "Failed to allocate WiFiServer on port %u", this->port_);
    this->mark_failed();
    return;
  }
  this->server_->begin();
  ESP_LOGI(TAG, "Modbus TCP server started on port %u", this->port_);
}

void SunSpecModbusServer::handle_client_() {
  uint32_t now = millis();

  // Check for new client
  if (this->server_->hasClient()) {
    if (this->client_connected_) {
      // Already have a client, reject new one
      WiFiClient new_client = this->server_->accept();
      new_client.stop();
      ESP_LOGW(TAG, "Rejected new client, already connected");
    } else {
      this->client_ = this->server_->accept();
      this->client_connected_ = true;
      this->last_rx_ms_ = now;
      ESP_LOGI(TAG, "Client connected from %s", this->client_.remoteIP().toString().c_str());
    }
  }

  // Handle existing client
  if (this->client_connected_) {
    // Stale connection timeout: force-close if no data received for CLIENT_TIMEOUT_MS
    bool timed_out = (now - this->last_rx_ms_) >= CLIENT_TIMEOUT_MS;

    if (!this->client_.connected() || timed_out) {
      if (timed_out) {
        ESP_LOGW(TAG, "Client timeout — forcing disconnect");
      } else {
        ESP_LOGI(TAG, "Client disconnected");
      }
      this->client_.stop();
      this->client_connected_ = false;
      return;
    }

    // Check for incoming data
    if (this->client_.available() >= MIN_REQUEST_SIZE) {
      uint8_t buffer[256];
      size_t len = this->client_.read(buffer, sizeof(buffer));
      if (len >= MIN_REQUEST_SIZE) {
        this->last_rx_ms_ = now;
        this->process_request_(this->client_, buffer, len);
      }
    }
  }
}

void SunSpecModbusServer::process_request_(WiFiClient &client, uint8_t *buffer, size_t len) {
  // Parse MBAP header
  // uint16_t transaction_id = (buffer[0] << 8) | buffer[1];
  uint16_t protocol_id = (buffer[2] << 8) | buffer[3];
  // uint16_t length = (buffer[4] << 8) | buffer[5];
  uint8_t unit_id = buffer[6];

  // Modbus TCP protocol_id must always be 0x0000
  if (protocol_id != 0) {
    ESP_LOGW(TAG, "Ignoring non-Modbus frame (protocol_id=0x%04X)", protocol_id);
    return;
  }
  uint8_t function_code = buffer[7];
  uint16_t start_addr = (buffer[8] << 8) | buffer[9];
  uint16_t quantity = (buffer[10] << 8) | buffer[11];

  ESP_LOGD(TAG, "Request: Unit=%u, FC=%u, Addr=%u, Qty=%u", unit_id, function_code, start_addr, quantity);

  // Check unit ID
  if (unit_id != this->unit_id_ && unit_id != 0) {
    // Ignore requests not for us (don't respond per Modbus spec)
    ESP_LOGD(TAG, "Ignoring request for unit %u", unit_id);
    return;
  }

  // Handle function codes
  switch (function_code) {
    case FC_READ_HOLDING_REGISTERS:
    case FC_READ_INPUT_REGISTERS: {
      // Convert Modbus address to register index
      uint16_t reg_start;
      if (start_addr >= SUNSPEC_BASE_ADDRESS) {
        reg_start = start_addr - SUNSPEC_BASE_ADDRESS;
      } else {
        // Some clients use 0-based addressing
        reg_start = start_addr;
      }

      // Validate address range
      if (reg_start + quantity > TOTAL_REGISTERS) {
        ESP_LOGW(TAG, "Invalid address range: %u + %u > %u", reg_start, quantity, TOTAL_REGISTERS);
        this->send_error_(client, buffer, EX_ILLEGAL_DATA_ADDRESS);
        return;
      }

      // Send response
      this->send_response_(client, buffer, reg_start, quantity);
      break;
    }
    case FC_WRITE_SINGLE_REGISTER: {
      this->handle_write_single_(client, buffer);
      break;
    }
    case FC_WRITE_MULTIPLE_REGISTERS: {
      this->handle_write_multiple_(client, buffer, len);
      break;
    }
    default:
      ESP_LOGW(TAG, "Unsupported function code: %u", function_code);
      this->send_error_(client, buffer, EX_ILLEGAL_FUNCTION);
      break;
  }
}

void SunSpecModbusServer::send_response_(WiFiClient &client, uint8_t *request, uint16_t start_addr, uint16_t reg_count) {
  // Build response — Modbus FC03 max is 125 registers (250 bytes data + 9 header = 259 bytes)
  static const uint16_t MAX_REGISTERS_PER_READ = 125;
  if (reg_count > MAX_REGISTERS_PER_READ) reg_count = MAX_REGISTERS_PER_READ;
  uint8_t response[9 + MAX_REGISTERS_PER_READ * 2];
  size_t response_len = MBAP_HEADER_SIZE + 2 + (reg_count * 2);

  // Copy MBAP header (transaction ID, protocol ID)
  memcpy(response, request, 4);

  // Length field (unit ID + function code + byte count + data)
  uint16_t length = 3 + (reg_count * 2);
  response[4] = length >> 8;
  response[5] = length & 0xFF;

  // Unit ID
  response[6] = request[6];

  // Function code (same as request)
  response[7] = request[7];

  // Byte count
  response[8] = reg_count * 2;

  // Register data (big-endian)
  for (uint16_t i = 0; i < reg_count; i++) {
    uint16_t reg_value = this->registers_[start_addr + i];
    response[9 + (i * 2)] = reg_value >> 8;
    response[9 + (i * 2) + 1] = reg_value & 0xFF;
  }

  client.write(response, response_len);
  ESP_LOGD(TAG, "Sent %u registers starting at %u", reg_count, start_addr);
}

void SunSpecModbusServer::send_error_(WiFiClient &client, uint8_t *request, uint8_t error_code) {
  uint8_t response[9];

  // Copy MBAP header
  memcpy(response, request, 4);

  // Length field
  response[4] = 0;
  response[5] = 3;

  // Unit ID
  response[6] = request[6];

  // Function code with error bit set
  response[7] = request[7] | 0x80;

  // Exception code
  response[8] = error_code;

  client.write(response, 9);
  ESP_LOGD(TAG, "Sent error response: %u", error_code);
}

void SunSpecModbusServer::handle_write_single_(WiFiClient &client, uint8_t *buffer) {
  uint16_t reg_addr = (buffer[8] << 8) | buffer[9];
  uint16_t value = (buffer[10] << 8) | buffer[11];

  // Convert to internal index
  uint16_t reg_idx = (reg_addr >= SUNSPEC_BASE_ADDRESS) ? reg_addr - SUNSPEC_BASE_ADDRESS : reg_addr;

  if (reg_idx >= TOTAL_REGISTERS) {
    this->send_error_(client, buffer, EX_ILLEGAL_DATA_ADDRESS);
    return;
  }

  this->registers_[reg_idx] = value;
  this->process_write_(reg_idx, 1);

  // Echo the request as response (FC06 standard)
  client.write(buffer, 12);
  ESP_LOGD(TAG, "Write single reg %u = %u", reg_idx, value);
}

void SunSpecModbusServer::handle_write_multiple_(WiFiClient &client, uint8_t *buffer, size_t len) {
  uint16_t reg_addr = (buffer[8] << 8) | buffer[9];
  uint16_t quantity = (buffer[10] << 8) | buffer[11];
  // buffer[12] = byte count

  uint16_t reg_idx = (reg_addr >= SUNSPEC_BASE_ADDRESS) ? reg_addr - SUNSPEC_BASE_ADDRESS : reg_addr;

  if (reg_idx + quantity > TOTAL_REGISTERS) {
    this->send_error_(client, buffer, EX_ILLEGAL_DATA_ADDRESS);
    return;
  }

  // Write register values — track actual count in case buffer is shorter than declared quantity
  uint16_t written = 0;
  for (uint16_t i = 0; i < quantity && (size_t)(13 + i * 2 + 1) < len; i++) {
    uint16_t value = (buffer[13 + i * 2] << 8) | buffer[14 + i * 2];
    this->registers_[reg_idx + i] = value;
    written++;
  }
  this->process_write_(reg_idx, written);

  // Send FC16 response: MBAP + unit + FC + start_addr + quantity
  uint8_t response[12];
  memcpy(response, buffer, 4);  // Transaction + protocol ID
  response[4] = 0;
  response[5] = 6;  // Length = 6
  response[6] = buffer[6];  // Unit ID
  response[7] = FC_WRITE_MULTIPLE_REGISTERS;
  response[8] = buffer[8];  // Start address hi
  response[9] = buffer[9];  // Start address lo
  response[10] = buffer[10]; // Quantity hi
  response[11] = buffer[11]; // Quantity lo
  client.write(response, 12);
  ESP_LOGD(TAG, "Write multiple %u regs starting at %u", quantity, reg_idx);
}

void SunSpecModbusServer::process_write_(uint16_t reg_start, uint16_t reg_count) {
  // Check if any Model 123 registers were touched
  if (reg_start + reg_count <= MODEL123_DATA_OFFSET) return;
  if (reg_start >= MODEL123_DATA_OFFSET + MODEL123_LENGTH) return;

  // Model 123 WMaxLimPct / WMaxLim_Ena control
  uint16_t ena = this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLim_Ena];
  uint16_t pct_raw = this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct];
  uint16_t rvrt_tms = this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct_RvrtTms];
  int16_t sf = (int16_t)this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct_SF];

  // Apply scale factor: value * 10^sf
  float pct;
  if (sf == 0) {
    pct = (float)pct_raw;
  } else {
    pct = (float)pct_raw * powf(10.0f, (float)sf);
  }
  if (pct < 0.0f) pct = 0.0f;
  if (pct > 100.0f) pct = 100.0f;

  // Arm or disarm the revert timer
  if (ena == 1 && rvrt_tms > 0) {
    this->revert_active_ = true;
    this->revert_deadline_ = millis() + ((uint32_t)rvrt_tms * 1000);
    ESP_LOGD(TAG, "Revert timer armed: %u seconds", rvrt_tms);
  } else {
    this->revert_active_ = false;
  }

  if (this->power_limit_number_ != nullptr) {
    float target = (ena == 1) ? pct : 100.0f;  // Disabled = restore full power
    ESP_LOGI(TAG, "Model123: WMaxLim_Ena=%u WMaxLimPct=%.1f%% -> setting Growatt to %.1f%%", ena, pct, target);
    auto call = this->power_limit_number_->make_call();
    call.set_value(target);
    call.perform();
  }
}

void SunSpecModbusServer::init_registers_() {
  // SunSpec identifier "SunS" (0x5375, 0x6E53)
  this->registers_[SUNSPEC_ID_OFFSET] = 0x5375;      // "Su"
  this->registers_[SUNSPEC_ID_OFFSET + 1] = 0x6E53;  // "nS"

  // Model 1 header (Common)
  this->registers_[MODEL1_ID_OFFSET] = 1;          // Model ID
  this->registers_[MODEL1_LENGTH_OFFSET] = MODEL1_LENGTH;  // Length

  // Model 1 data - Manufacturer info
  this->write_string_(MODEL1_DATA_OFFSET + 0, this->manufacturer_.c_str(), 32);   // Mn (offset 0-15)
  this->write_string_(MODEL1_DATA_OFFSET + 16, this->model_.c_str(), 32);          // Md (offset 16-31)
  this->write_string_(MODEL1_DATA_OFFSET + 32, "", 16);                            // Opt (offset 32-39)
  this->write_string_(MODEL1_DATA_OFFSET + 40, this->version_.c_str(), 16);         // Vr (offset 40-47)
  this->write_string_(MODEL1_DATA_OFFSET + 48, this->serial_.c_str(), 32);         // SN (offset 48-63)
  this->registers_[MODEL1_DATA_OFFSET + 64] = 1;                                   // DA (Device Address)

  // Model 120 header (Nameplate Ratings)
  this->registers_[MODEL120_ID_OFFSET] = 120;
  this->registers_[MODEL120_LENGTH_OFFSET] = MODEL120_LENGTH;

  // Model 120 data
  this->registers_[MODEL120_DATA_OFFSET + Model120::DERTyp]   = 4;                  // PV device
  this->registers_[MODEL120_DATA_OFFSET + Model120::WRtg]     = this->max_power_;   // Rated power (W)
  this->registers_[MODEL120_DATA_OFFSET + Model120::WRtg_SF]  = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::VARtg]    = this->max_power_;   // Rated VA (same as W for unity PF)
  this->registers_[MODEL120_DATA_OFFSET + Model120::VARtg_SF] = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::VArRtgQ1] = 0;                  // PV-only: no reactive power
  this->registers_[MODEL120_DATA_OFFSET + Model120::VArRtgQ2] = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::VArRtgQ3] = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::VArRtgQ4] = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::VArRtg_SF] = 0;
  // ARtg: max current sum of 3 phases = WRtg / (sqrt(3) * 400V) * 3
  this->registers_[MODEL120_DATA_OFFSET + Model120::ARtg]     = (uint16_t)(this->max_power_ / 230.94f);
  this->registers_[MODEL120_DATA_OFFSET + Model120::ARtg_SF]  = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::PFRtgQ1]  = (uint16_t)(int16_t)100;  // 1.00 with SF=-2
  this->registers_[MODEL120_DATA_OFFSET + Model120::PFRtgQ2]  = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::PFRtgQ3]  = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::PFRtgQ4]  = 0;
  this->registers_[MODEL120_DATA_OFFSET + Model120::PFRtg_SF] = (uint16_t)(int16_t)(-2);
  // Optional storage fields (17-25) left as 0 — not applicable for PV

  // Model 103 header (Three-Phase Inverter)
  this->registers_[MODEL103_ID_OFFSET] = 103;       // Model ID
  this->registers_[MODEL103_LENGTH_OFFSET] = MODEL103_LENGTH;  // Length

  // Model 103 scale factors (set once, don't change)
  this->registers_[MODEL103_DATA_OFFSET + Model103::A_SF] = (uint16_t)(int16_t)(-2);   // Current: 0.01A resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::V_SF] = (uint16_t)(int16_t)(-1);   // Voltage: 0.1V resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::W_SF] = 0;                          // Power: 1W resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::Hz_SF] = (uint16_t)(int16_t)(-2);  // Frequency: 0.01Hz resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::VA_SF] = 0;                         // VA: 1VA resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::VAr_SF] = 0;                        // VAr: 1VAr resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::PF_SF] = (uint16_t)(int16_t)(-2);  // PF: 0.01 resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::WH_SF] = 0;                         // Energy: 1Wh resolution
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCA_SF] = (uint16_t)(int16_t)(-2); // DC Current: 0.01A
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCV_SF] = (uint16_t)(int16_t)(-1); // DC Voltage: 0.1V
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCW_SF] = 0;                        // DC Power: 1W
  this->registers_[MODEL103_DATA_OFFSET + Model103::Tmp_SF] = 0;                        // Temperature: 1°C

  // Initialize DC voltage (always present when connected)
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCV] = 4500;  // 450.0V

  // Model 160 (Multiple MPPT) Header
  this->registers_[MODEL160_ID_OFFSET] = 160;
  this->registers_[MODEL160_LENGTH_OFFSET] = MODEL160_LENGTH;

  // Model 160 global scale factors
  this->registers_[MODEL160_DATA_OFFSET + Model160::DCA_SF]  = (uint16_t)(int16_t)(-2);  // 0.01A
  this->registers_[MODEL160_DATA_OFFSET + Model160::DCV_SF]  = (uint16_t)(int16_t)(-1);  // 0.1V
  this->registers_[MODEL160_DATA_OFFSET + Model160::DCW_SF]  = 0;                         // 1W
  this->registers_[MODEL160_DATA_OFFSET + Model160::DCWH_SF] = 0;
  this->registers_[MODEL160_DATA_OFFSET + Model160::N]       = 2;  // PV1 + PV2

  // Tracker 0 (PV1) ID and IDStr ("PV1")
  uint16_t t0 = MODEL160_DATA_OFFSET + MODEL160_TRACKER_BASE + 0 * MODEL160_TRACKER_STRIDE;
  this->registers_[t0 + Model160::T_ID] = 1;
  this->write_string_(t0 + 1, "PV1", 16);  // IDStr: 8 registers

  // Tracker 1 (PV2) ID and IDStr ("PV2")
  uint16_t t1 = MODEL160_DATA_OFFSET + MODEL160_TRACKER_BASE + 1 * MODEL160_TRACKER_STRIDE;
  this->registers_[t1 + Model160::T_ID] = 2;
  this->write_string_(t1 + 1, "PV2", 16);  // IDStr: 8 registers

  // Model 123 (Immediate Controls) Header
  this->registers_[MODEL123_ID_OFFSET] = 123;
  this->registers_[MODEL123_LENGTH_OFFSET] = MODEL123_LENGTH;

  // Model 123 scale factors
  this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct_SF] = 0;   // Direct % (0-100)
  this->registers_[MODEL123_DATA_OFFSET + Model123::OutPFSet_SF] = (uint16_t)(int16_t)(-2);

  // Default: no power limit (100%), disabled
  this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLimPct] = 100;
  this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLim_Ena] = 0;

  // End model marker (now at offset 147)
  this->registers_[END_MODEL_OFFSET] = 0xFFFF;
  this->registers_[END_MODEL_OFFSET + 1] = 0;

  ESP_LOGI(TAG, "SunSpec registers initialized");
}

void SunSpecModbusServer::update_registers_() {
  // Update Model 103 registers with current simulated values

  // AC Current (scale factor -2, so multiply by 100)
  this->registers_[MODEL103_DATA_OFFSET + Model103::A]    = safe_u16(this->values_.ac_current_total * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphA] = safe_u16(this->values_.ac_current_a * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphB] = safe_u16(this->values_.ac_current_b * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphC] = safe_u16(this->values_.ac_current_c * 100);

  // Line voltages (phase-to-phase, scale factor -1, multiply by 10)
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphAB] = safe_u16(this->values_.line_voltage_ab * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphBC] = safe_u16(this->values_.line_voltage_bc * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphCA] = safe_u16(this->values_.line_voltage_ca * 10);

  // Phase voltages (phase-to-neutral, scale factor -1, multiply by 10)
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphA] = safe_u16(this->values_.ac_voltage_a * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphB] = safe_u16(this->values_.ac_voltage_b * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphC] = safe_u16(this->values_.ac_voltage_c * 10);

  // AC Power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::W] = safe_u16(this->values_.ac_power);

  // Frequency (scale factor -2, multiply by 100)
  this->registers_[MODEL103_DATA_OFFSET + Model103::Hz] = safe_u16(this->values_.frequency * 100);

  // Apparent power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::VA] = safe_u16(this->values_.apparent_power);

  // Reactive power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::VAr] = safe_u16(this->values_.reactive_power);

  // Power factor (scale factor -2, multiply by 100, signed: clamp to [-100, 100])
  float pf_scaled = this->values_.power_factor * 100.0f;
  int16_t pf_reg = std::isfinite(pf_scaled) ? static_cast<int16_t>(std::max(-100.0f, std::min(100.0f, pf_scaled))) : 0;
  this->registers_[MODEL103_DATA_OFFSET + Model103::PF] = static_cast<uint16_t>(pf_reg);

  // Energy (32-bit, scale factor 0)
  this->write_uint32_(MODEL103_DATA_OFFSET + Model103::WH_HI, this->values_.total_energy);

  // DC values
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCA] = safe_u16(this->values_.dc_current * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCV] = safe_u16(this->values_.dc_voltage * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCW] = safe_u16(this->values_.dc_power);

  // Temperature
  this->registers_[MODEL103_DATA_OFFSET + Model103::TmpCab] = static_cast<uint16_t>(this->values_.temperature);
  this->registers_[MODEL103_DATA_OFFSET + Model103::TmpSnk] = static_cast<uint16_t>(this->values_.temperature);

  // Operating state
  this->registers_[MODEL103_DATA_OFFSET + Model103::St] = static_cast<uint16_t>(this->values_.state);

  // Model 160 — tracker live data
  // Tracker 0 (PV1) — reuse existing DC source values
  uint16_t t0 = MODEL160_DATA_OFFSET + MODEL160_TRACKER_BASE + 0 * MODEL160_TRACKER_STRIDE;
  this->registers_[t0 + Model160::T_DCA] = safe_u16(this->values_.dc_current * 100);
  this->registers_[t0 + Model160::T_DCV] = safe_u16(this->values_.dc_voltage * 10);
  this->registers_[t0 + Model160::T_DCW] = safe_u16(this->values_.dc_power);

  // Tracker 1 (PV2) — from optional PV2 source sensors
  uint16_t t1 = MODEL160_DATA_OFFSET + MODEL160_TRACKER_BASE + 1 * MODEL160_TRACKER_STRIDE;
  if (this->source_pv2_voltage_ != nullptr && this->source_pv2_voltage_->has_state()) {
    this->registers_[t1 + Model160::T_DCV] = safe_u16(this->source_pv2_voltage_->state * 10);
  }
  if (this->source_pv2_current_ != nullptr && this->source_pv2_current_->has_state()) {
    this->registers_[t1 + Model160::T_DCA] = safe_u16(this->source_pv2_current_->state * 100);
  }
  if (this->source_pv2_power_ != nullptr && this->source_pv2_power_->has_state()) {
    this->registers_[t1 + Model160::T_DCW] = safe_u16(this->source_pv2_power_->state);
  }
}

void SunSpecModbusServer::write_string_(uint16_t offset, const char *str, uint16_t max_len) {
  size_t str_len = strlen(str);
  uint16_t reg_count = max_len / 2;

  for (uint16_t i = 0; i < reg_count; i++) {
    uint8_t high_byte = (i * 2 < str_len) ? (uint8_t)str[i * 2] : 0;
    uint8_t low_byte = (i * 2 + 1 < str_len) ? (uint8_t)str[i * 2 + 1] : 0;
    this->registers_[offset + i] = (high_byte << 8) | low_byte;
  }
}

void SunSpecModbusServer::write_uint32_(uint16_t offset, uint32_t value) {
  this->registers_[offset] = (value >> 16) & 0xFFFF;      // High word
  this->registers_[offset + 1] = value & 0xFFFF;          // Low word
}

void SunSpecModbusServer::publish_sensors_() {
  if (this->ac_power_sensor_ != nullptr)
    this->ac_power_sensor_->publish_state(this->values_.ac_power);

  if (this->ac_voltage_a_sensor_ != nullptr)
    this->ac_voltage_a_sensor_->publish_state(this->values_.ac_voltage_a);

  if (this->ac_voltage_b_sensor_ != nullptr)
    this->ac_voltage_b_sensor_->publish_state(this->values_.ac_voltage_b);

  if (this->ac_voltage_c_sensor_ != nullptr)
    this->ac_voltage_c_sensor_->publish_state(this->values_.ac_voltage_c);

  if (this->ac_current_a_sensor_ != nullptr)
    this->ac_current_a_sensor_->publish_state(this->values_.ac_current_a);

  if (this->ac_current_b_sensor_ != nullptr)
    this->ac_current_b_sensor_->publish_state(this->values_.ac_current_b);

  if (this->ac_current_c_sensor_ != nullptr)
    this->ac_current_c_sensor_->publish_state(this->values_.ac_current_c);

  if (this->ac_current_total_sensor_ != nullptr)
    this->ac_current_total_sensor_->publish_state(this->values_.ac_current_total);

  if (this->frequency_sensor_ != nullptr)
    this->frequency_sensor_->publish_state(this->values_.frequency);

  if (this->power_factor_sensor_ != nullptr)
    this->power_factor_sensor_->publish_state(this->values_.power_factor);

  if (this->total_energy_sensor_ != nullptr)
    this->total_energy_sensor_->publish_state(this->values_.total_energy);

  if (this->dc_voltage_sensor_ != nullptr)
    this->dc_voltage_sensor_->publish_state(this->values_.dc_voltage);

  if (this->dc_current_sensor_ != nullptr)
    this->dc_current_sensor_->publish_state(this->values_.dc_current);

  if (this->dc_power_sensor_ != nullptr)
    this->dc_power_sensor_->publish_state(this->values_.dc_power);

  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(this->values_.temperature);
}

void SunSpecModbusServer::update_from_sources_() {
  // Read values from external source sensors (e.g., from modbus_controller)
  // Only update values if the source sensor exists and has a valid state

  // AC Power
  if (this->source_ac_power_ != nullptr && this->source_ac_power_->has_state()) {
    this->values_.ac_power = this->source_ac_power_->state;
  }

  // Phase voltages
  if (this->source_voltage_a_ != nullptr && this->source_voltage_a_->has_state()) {
    this->values_.ac_voltage_a = this->source_voltage_a_->state;
  }
  if (this->source_voltage_b_ != nullptr && this->source_voltage_b_->has_state()) {
    this->values_.ac_voltage_b = this->source_voltage_b_->state;
  }
  if (this->source_voltage_c_ != nullptr && this->source_voltage_c_->has_state()) {
    this->values_.ac_voltage_c = this->source_voltage_c_->state;
  }

  // Calculate line voltages from phase voltages (phase-to-phase = phase * sqrt(3))
  this->values_.line_voltage_ab = this->values_.ac_voltage_a * 1.732f;
  this->values_.line_voltage_bc = this->values_.ac_voltage_b * 1.732f;
  this->values_.line_voltage_ca = this->values_.ac_voltage_c * 1.732f;

  // Phase currents
  if (this->source_current_a_ != nullptr && this->source_current_a_->has_state()) {
    this->values_.ac_current_a = this->source_current_a_->state;
  }
  if (this->source_current_b_ != nullptr && this->source_current_b_->has_state()) {
    this->values_.ac_current_b = this->source_current_b_->state;
  }
  if (this->source_current_c_ != nullptr && this->source_current_c_->has_state()) {
    this->values_.ac_current_c = this->source_current_c_->state;
  }

  // Total current = sum of phase currents
  this->values_.ac_current_total = this->values_.ac_current_a +
                                    this->values_.ac_current_b +
                                    this->values_.ac_current_c;

  // Frequency
  if (this->source_frequency_ != nullptr && this->source_frequency_->has_state()) {
    this->values_.frequency = this->source_frequency_->state;
  }

  // Power factor
  if (this->source_power_factor_ != nullptr && this->source_power_factor_->has_state()) {
    this->values_.power_factor = this->source_power_factor_->state;
  }

  // Calculate apparent power (VA) = P / PF
  if (this->values_.power_factor > 0) {
    this->values_.apparent_power = this->values_.ac_power / this->values_.power_factor;
  } else {
    this->values_.apparent_power = this->values_.ac_power;
  }

  // Calculate reactive power (VAr) = sqrt(VA^2 - W^2)
  float va_squared = this->values_.apparent_power * this->values_.apparent_power;
  float w_squared = this->values_.ac_power * this->values_.ac_power;
  if (va_squared > w_squared) {
    this->values_.reactive_power = sqrtf(va_squared - w_squared);
  } else {
    this->values_.reactive_power = 0;
  }

  // Total energy (Wh)
  if (this->source_total_energy_ != nullptr && this->source_total_energy_->has_state()) {
    this->values_.total_energy = (uint32_t)this->source_total_energy_->state;
  }

  // DC voltage
  if (this->source_dc_voltage_ != nullptr && this->source_dc_voltage_->has_state()) {
    this->values_.dc_voltage = this->source_dc_voltage_->state;
  }

  // DC current
  if (this->source_dc_current_ != nullptr && this->source_dc_current_->has_state()) {
    this->values_.dc_current = this->source_dc_current_->state;
  }

  // DC power - read from source or calculate from V*I
  if (this->source_dc_power_ != nullptr && this->source_dc_power_->has_state()) {
    this->values_.dc_power = this->source_dc_power_->state;
  } else if (this->values_.dc_voltage > 0 && this->values_.dc_current > 0) {
    this->values_.dc_power = this->values_.dc_voltage * this->values_.dc_current;
  }

  // Temperature
  if (this->source_temperature_ != nullptr && this->source_temperature_->has_state()) {
    this->values_.temperature = (int16_t)this->source_temperature_->state;
  }

  // Set operating state based on power and active throttle
  bool throttled = (this->registers_[MODEL123_DATA_OFFSET + Model123::WMaxLim_Ena] == 1);
  if (this->values_.ac_power > 0) {
    this->values_.state = throttled ? InverterState::THROTTLED : InverterState::MPPT;
  } else {
    this->values_.state = InverterState::STANDBY;
  }
}

}  // namespace sunspec_modbus_server
}  // namespace esphome
