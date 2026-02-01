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

// Modbus exception codes
static const uint8_t EX_ILLEGAL_FUNCTION = 0x01;
static const uint8_t EX_ILLEGAL_DATA_ADDRESS = 0x02;
static const uint8_t EX_ILLEGAL_DATA_VALUE = 0x03;
static const uint8_t EX_SERVER_DEVICE_FAILURE = 0x04;

// Modbus TCP header size (MBAP)
static const size_t MBAP_HEADER_SIZE = 7;
static const size_t MIN_REQUEST_SIZE = 12;  // MBAP + Unit ID + FC + Start Addr + Quantity

SunSpecModbusServer::SunSpecModbusServer() {
  memset(this->registers_, 0, sizeof(this->registers_));
}

void SunSpecModbusServer::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SunSpec Modbus TCP Server...");

  // Initialize SunSpec registers with static data
  this->init_registers_();

  // Initialize simulation
  this->start_time_ = millis();
  this->last_update_ = this->start_time_;
  this->accumulated_energy_ = 0;

  // Initialize simulated values
  this->values_.ac_voltage_a = this->grid_voltage_;
  this->values_.ac_voltage_b = this->grid_voltage_;
  this->values_.ac_voltage_c = this->grid_voltage_;
  this->values_.line_voltage_ab = this->grid_voltage_ * 1.732f;
  this->values_.line_voltage_bc = this->grid_voltage_ * 1.732f;
  this->values_.line_voltage_ca = this->grid_voltage_ * 1.732f;
  this->values_.frequency = this->grid_frequency_;
  this->values_.power_factor = 0.99f;
  this->values_.temperature = 35;
  this->values_.dc_voltage = 450.0f;

  // Start TCP server
  this->start_server_();
}

void SunSpecModbusServer::loop() {
  // Update simulation
  uint32_t now = millis();
  if (now - this->last_update_ >= this->update_interval_) {
    this->update_simulation_();
    this->update_registers_();
    this->publish_sensors_();
    this->last_update_ = now;
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
  ESP_LOGCONFIG(TAG, "  Max Power: %u W", this->max_power_);
  ESP_LOGCONFIG(TAG, "  Grid Voltage: %.1f V", this->grid_voltage_);
  ESP_LOGCONFIG(TAG, "  Grid Frequency: %.1f Hz", this->grid_frequency_);
  ESP_LOGCONFIG(TAG, "  Update Interval: %u ms", this->update_interval_);
}

void SunSpecModbusServer::start_server_() {
  this->server_ = new WiFiServer(this->port_);
  this->server_->begin();
  ESP_LOGI(TAG, "Modbus TCP server started on port %u", this->port_);
}

void SunSpecModbusServer::handle_client_() {
  // Check for new client
  if (this->server_->hasClient()) {
    if (this->client_connected_) {
      // Already have a client, reject new one
      WiFiClient new_client = this->server_->available();
      new_client.stop();
      ESP_LOGW(TAG, "Rejected new client, already connected");
    } else {
      this->client_ = this->server_->available();
      this->client_connected_ = true;
      ESP_LOGI(TAG, "Client connected from %s", this->client_.remoteIP().toString().c_str());
    }
  }

  // Handle existing client
  if (this->client_connected_) {
    if (!this->client_.connected()) {
      this->client_connected_ = false;
      ESP_LOGI(TAG, "Client disconnected");
      return;
    }

    // Check for incoming data
    if (this->client_.available() >= MIN_REQUEST_SIZE) {
      uint8_t buffer[256];
      size_t len = this->client_.read(buffer, sizeof(buffer));
      if (len >= MIN_REQUEST_SIZE) {
        this->process_request_(this->client_, buffer, len);
      }
    }
  }
}

void SunSpecModbusServer::process_request_(WiFiClient &client, uint8_t *buffer, size_t len) {
  // Parse MBAP header
  // uint16_t transaction_id = (buffer[0] << 8) | buffer[1];
  // uint16_t protocol_id = (buffer[2] << 8) | buffer[3];
  // uint16_t length = (buffer[4] << 8) | buffer[5];
  uint8_t unit_id = buffer[6];
  uint8_t function_code = buffer[7];
  uint16_t start_addr = (buffer[8] << 8) | buffer[9];
  uint16_t quantity = (buffer[10] << 8) | buffer[11];

  ESP_LOGI(TAG, "Request: Unit=%u, FC=%u, Addr=%u, Qty=%u", unit_id, function_code, start_addr, quantity);

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
    default:
      ESP_LOGW(TAG, "Unsupported function code: %u", function_code);
      this->send_error_(client, buffer, EX_ILLEGAL_FUNCTION);
      break;
  }
}

void SunSpecModbusServer::send_response_(WiFiClient &client, uint8_t *request, uint16_t start_addr, uint16_t reg_count) {
  // Build response
  uint8_t response[256];
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
  ESP_LOGI(TAG, "Sent %u registers starting at %u", reg_count, start_addr);
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
  this->write_string_(MODEL1_DATA_OFFSET + 40, "1.0.0", 16);                       // Vr (offset 40-47)
  this->write_string_(MODEL1_DATA_OFFSET + 48, this->serial_.c_str(), 32);         // SN (offset 48-63)
  this->registers_[MODEL1_DATA_OFFSET + 64] = 1;                                   // DA (Device Address)

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

  // End model marker
  this->registers_[END_MODEL_OFFSET] = 0xFFFF;
  this->registers_[END_MODEL_OFFSET + 1] = 0;

  ESP_LOGI(TAG, "SunSpec registers initialized");
}

void SunSpecModbusServer::update_registers_() {
  // Update Model 103 registers with current simulated values

  // AC Current (scale factor -2, so multiply by 100)
  this->registers_[MODEL103_DATA_OFFSET + Model103::A] = (uint16_t)(this->values_.ac_current_total * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphA] = (uint16_t)(this->values_.ac_current_a * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphB] = (uint16_t)(this->values_.ac_current_b * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::AphC] = (uint16_t)(this->values_.ac_current_c * 100);

  // Line voltages (phase-to-phase, scale factor -1, multiply by 10)
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphAB] = (uint16_t)(this->values_.line_voltage_ab * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphBC] = (uint16_t)(this->values_.line_voltage_bc * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PPVphCA] = (uint16_t)(this->values_.line_voltage_ca * 10);

  // Phase voltages (phase-to-neutral, scale factor -1, multiply by 10)
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphA] = (uint16_t)(this->values_.ac_voltage_a * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphB] = (uint16_t)(this->values_.ac_voltage_b * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::PhVphC] = (uint16_t)(this->values_.ac_voltage_c * 10);

  // AC Power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::W] = (uint16_t)this->values_.ac_power;

  // Frequency (scale factor -2, multiply by 100)
  this->registers_[MODEL103_DATA_OFFSET + Model103::Hz] = (uint16_t)(this->values_.frequency * 100);

  // Apparent power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::VA] = (uint16_t)this->values_.apparent_power;

  // Reactive power (scale factor 0)
  this->registers_[MODEL103_DATA_OFFSET + Model103::VAr] = (uint16_t)this->values_.reactive_power;

  // Power factor (scale factor -2, multiply by 100, range -1.00 to 1.00 = -100 to 100)
  this->registers_[MODEL103_DATA_OFFSET + Model103::PF] = (uint16_t)(int16_t)(this->values_.power_factor * 100);

  // Energy (32-bit, scale factor 0)
  this->write_uint32_(MODEL103_DATA_OFFSET + Model103::WH_HI, this->values_.total_energy);

  // DC values
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCA] = (uint16_t)(this->values_.dc_current * 100);
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCV] = (uint16_t)(this->values_.dc_voltage * 10);
  this->registers_[MODEL103_DATA_OFFSET + Model103::DCW] = (uint16_t)this->values_.dc_power;

  // Temperature
  this->registers_[MODEL103_DATA_OFFSET + Model103::TmpCab] = (uint16_t)this->values_.temperature;
  this->registers_[MODEL103_DATA_OFFSET + Model103::TmpSnk] = (uint16_t)this->values_.temperature;

  // Operating state
  this->registers_[MODEL103_DATA_OFFSET + Model103::St] = static_cast<uint16_t>(this->values_.state);
}

void SunSpecModbusServer::write_string_(uint16_t offset, const char *str, uint16_t max_len) {
  size_t str_len = strlen(str);
  uint16_t reg_count = max_len / 2;

  for (uint16_t i = 0; i < reg_count; i++) {
    uint8_t high_byte = (i * 2 < str_len) ? str[i * 2] : ' ';
    uint8_t low_byte = (i * 2 + 1 < str_len) ? str[i * 2 + 1] : ' ';
    this->registers_[offset + i] = (high_byte << 8) | low_byte;
  }
}

void SunSpecModbusServer::write_uint32_(uint16_t offset, uint32_t value) {
  this->registers_[offset] = (value >> 16) & 0xFFFF;      // High word
  this->registers_[offset + 1] = value & 0xFFFF;          // Low word
}

void SunSpecModbusServer::update_simulation_() {
  // Calculate power based on time (simulated solar curve)
  this->values_.ac_power = this->calculate_power_();

  // Add noise to voltage readings
  this->values_.ac_voltage_a = this->add_noise_(this->grid_voltage_, 5.0f);
  this->values_.ac_voltage_b = this->add_noise_(this->grid_voltage_, 5.0f);
  this->values_.ac_voltage_c = this->add_noise_(this->grid_voltage_, 5.0f);

  // Line voltages (phase-to-phase) = phase voltage * sqrt(3)
  this->values_.line_voltage_ab = this->add_noise_(this->grid_voltage_ * 1.732f, 8.0f);
  this->values_.line_voltage_bc = this->add_noise_(this->grid_voltage_ * 1.732f, 8.0f);
  this->values_.line_voltage_ca = this->add_noise_(this->grid_voltage_ * 1.732f, 8.0f);

  // Frequency with small variation
  this->values_.frequency = this->add_noise_(this->grid_frequency_, 0.1f);

  // Calculate currents from power (balanced three-phase)
  if (this->values_.ac_power > 0) {
    float avg_line_voltage = (this->values_.line_voltage_ab + this->values_.line_voltage_bc +
                               this->values_.line_voltage_ca) / 3.0f;
    this->values_.ac_current_total = this->values_.ac_power / (1.732f * avg_line_voltage * this->values_.power_factor);

    // Phase currents (slightly unbalanced for realism)
    float phase_current = this->values_.ac_current_total / 3.0f;
    this->values_.ac_current_a = this->add_noise_(phase_current, phase_current * 0.02f);
    this->values_.ac_current_b = this->add_noise_(phase_current, phase_current * 0.02f);
    this->values_.ac_current_c = this->add_noise_(phase_current, phase_current * 0.02f);
    this->values_.ac_current_total = this->values_.ac_current_a + this->values_.ac_current_b + this->values_.ac_current_c;
    this->values_.state = InverterState::MPPT;
  } else {
    this->values_.ac_current_total = 0;
    this->values_.ac_current_a = 0;
    this->values_.ac_current_b = 0;
    this->values_.ac_current_c = 0;
    this->values_.state = InverterState::STANDBY;
  }

  // Power factor with small variation
  this->values_.power_factor = this->add_noise_(0.99f, 0.01f);
  if (this->values_.power_factor > 1.0f) this->values_.power_factor = 1.0f;
  if (this->values_.power_factor < 0.95f) this->values_.power_factor = 0.95f;

  // Apparent power (VA) = P / PF
  this->values_.apparent_power = this->values_.ac_power / this->values_.power_factor;

  // Reactive power (VAr) = sqrt(VA^2 - W^2)
  float va_squared = this->values_.apparent_power * this->values_.apparent_power;
  float w_squared = this->values_.ac_power * this->values_.ac_power;
  this->values_.reactive_power = sqrtf(va_squared - w_squared);

  // DC side calculations (typical inverter efficiency ~97%)
  float inverter_efficiency = 0.97f;
  this->values_.dc_power = this->values_.ac_power / inverter_efficiency;

  // DC voltage with noise
  this->values_.dc_voltage = this->add_noise_(450.0f, 20.0f);

  // DC current from power and voltage
  if (this->values_.dc_voltage > 0 && this->values_.dc_power > 0) {
    this->values_.dc_current = this->values_.dc_power / this->values_.dc_voltage;
  } else {
    this->values_.dc_current = 0;
  }

  // Temperature varies slightly with power
  float temp_rise = (this->values_.ac_power / (float)this->max_power_) * 15.0f;
  this->values_.temperature = (int16_t)(25 + temp_rise + this->add_noise_(0, 2.0f));

  // Accumulate energy (Wh)
  float energy_increment = this->values_.ac_power * (float)this->update_interval_ / 3600000.0f;
  this->accumulated_energy_ += (uint32_t)energy_increment;
  this->values_.total_energy = this->accumulated_energy_;
}

float SunSpecModbusServer::calculate_power_() {
  // Simulate a solar curve using time since start
  uint32_t elapsed_ms = millis() - this->start_time_;
  float elapsed_seconds = elapsed_ms / 1000.0f;

  // Complete one "day" cycle every 60 seconds for easy testing
  float cycle_seconds = 60.0f;
  float phase = (elapsed_seconds / cycle_seconds) * 2.0f * M_PI;

  // Sine wave from 0 to 1 (shifted to only positive values)
  float solar_factor = (sinf(phase - M_PI / 2) + 1.0f) / 2.0f;

  // Scale to max power with some noise
  float power = solar_factor * (float)this->max_power_;
  power = this->add_noise_(power, power * 0.02f);

  // Clamp to valid range
  if (power < 1) power = 0;
  if (power > (float)this->max_power_) power = (float)this->max_power_;

  return power;
}

float SunSpecModbusServer::add_noise_(float value, float max_noise) {
  if (max_noise <= 0) return value;

  // Simple random noise using ESP32's random
  float noise = ((float)(random(0, 2001) - 1000) / 1000.0f) * max_noise;
  return value + noise;
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

}  // namespace sunspec_modbus_server
}  // namespace esphome
