#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

#include <WiFi.h>
#include <vector>
#include <memory>

namespace esphome {
namespace sunspec_modbus_server {

// SunSpec Operating States (Model 103)
enum class InverterState : uint16_t {
  OFF = 1,
  SLEEPING = 2,
  STARTING = 3,
  MPPT = 4,
  THROTTLED = 5,
  SHUTTING_DOWN = 6,
  FAULT = 7,
  STANDBY = 8
};

// SunSpec register layout constants
static const uint16_t SUNSPEC_BASE_ADDRESS = 40000;
static const uint16_t SUNSPEC_ID_OFFSET = 0;
static const uint16_t MODEL1_ID_OFFSET = 2;
static const uint16_t MODEL1_LENGTH_OFFSET = 3;
static const uint16_t MODEL1_DATA_OFFSET = 4;
static const uint16_t MODEL1_LENGTH = 65;
static const uint16_t MODEL103_ID_OFFSET = 69;
static const uint16_t MODEL103_LENGTH_OFFSET = 70;
static const uint16_t MODEL103_DATA_OFFSET = 71;
static const uint16_t MODEL103_LENGTH = 50;
static const uint16_t END_MODEL_OFFSET = 121;
static const uint16_t TOTAL_REGISTERS = 123;

// Model 103 register offsets (relative to MODEL103_DATA_OFFSET)
namespace Model103 {
  static const uint8_t A = 0;        // AC Total Current
  static const uint8_t AphA = 1;     // Phase A Current
  static const uint8_t AphB = 2;     // Phase B Current
  static const uint8_t AphC = 3;     // Phase C Current
  static const uint8_t A_SF = 4;     // Current Scale Factor
  static const uint8_t PPVphAB = 5;  // Phase AB Voltage
  static const uint8_t PPVphBC = 6;  // Phase BC Voltage
  static const uint8_t PPVphCA = 7;  // Phase CA Voltage
  static const uint8_t PhVphA = 8;   // Phase A Voltage
  static const uint8_t PhVphB = 9;   // Phase B Voltage
  static const uint8_t PhVphC = 10;  // Phase C Voltage
  static const uint8_t V_SF = 11;    // Voltage Scale Factor
  static const uint8_t W = 12;       // AC Power
  static const uint8_t W_SF = 13;    // Power Scale Factor
  static const uint8_t Hz = 14;      // Frequency
  static const uint8_t Hz_SF = 15;   // Frequency Scale Factor
  static const uint8_t VA = 16;      // Apparent Power
  static const uint8_t VA_SF = 17;   // VA Scale Factor
  static const uint8_t VAr = 18;     // Reactive Power
  static const uint8_t VAr_SF = 19;  // VAr Scale Factor
  static const uint8_t PF = 20;      // Power Factor
  static const uint8_t PF_SF = 21;   // PF Scale Factor
  static const uint8_t WH_HI = 22;   // Energy High Word
  static const uint8_t WH_LO = 23;   // Energy Low Word
  static const uint8_t WH_SF = 24;   // Energy Scale Factor
  static const uint8_t DCA = 25;     // DC Current
  static const uint8_t DCA_SF = 26;  // DC Current SF
  static const uint8_t DCV = 27;     // DC Voltage
  static const uint8_t DCV_SF = 28;  // DC Voltage SF
  static const uint8_t DCW = 29;     // DC Power
  static const uint8_t DCW_SF = 30;  // DC Power SF
  static const uint8_t TmpCab = 31;  // Cabinet Temperature
  static const uint8_t TmpSnk = 32;  // Heat Sink Temperature
  static const uint8_t TmpTrns = 33; // Transformer Temperature
  static const uint8_t TmpOt = 34;   // Other Temperature
  static const uint8_t Tmp_SF = 35;  // Temperature Scale Factor
  static const uint8_t St = 36;      // Operating State
  static const uint8_t StVnd = 37;   // Vendor Operating State
}  // namespace Model103

// Simulated inverter values
struct SimulatedValues {
  float ac_power{0};
  float ac_voltage_a{230.0f};
  float ac_voltage_b{230.0f};
  float ac_voltage_c{230.0f};
  float line_voltage_ab{398.0f};
  float line_voltage_bc{398.0f};
  float line_voltage_ca{398.0f};
  float ac_current_total{0};
  float ac_current_a{0};
  float ac_current_b{0};
  float ac_current_c{0};
  float frequency{50.0f};
  float power_factor{0.99f};
  float apparent_power{0};
  float reactive_power{0};
  uint32_t total_energy{0};
  float dc_voltage{450.0f};
  float dc_current{0};
  float dc_power{0};
  int16_t temperature{35};
  InverterState state{InverterState::MPPT};
};

class SunSpecModbusServer : public Component {
 public:
  SunSpecModbusServer();

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  // Configuration setters
  void set_port(uint16_t port) { this->port_ = port; }
  void set_unit_id(uint8_t unit_id) { this->unit_id_ = unit_id; }
  void set_manufacturer(const std::string &manufacturer) { this->manufacturer_ = manufacturer; }
  void set_model(const std::string &model) { this->model_ = model; }
  void set_serial(const std::string &serial) { this->serial_ = serial; }
  void set_max_power(uint32_t max_power) { this->max_power_ = max_power; }
  void set_grid_voltage(float grid_voltage) { this->grid_voltage_ = grid_voltage; }
  void set_grid_frequency(float grid_frequency) { this->grid_frequency_ = grid_frequency; }
  void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }

  // Sensor setters
  void set_ac_power_sensor(sensor::Sensor *sensor) { this->ac_power_sensor_ = sensor; }
  void set_ac_voltage_a_sensor(sensor::Sensor *sensor) { this->ac_voltage_a_sensor_ = sensor; }
  void set_ac_voltage_b_sensor(sensor::Sensor *sensor) { this->ac_voltage_b_sensor_ = sensor; }
  void set_ac_voltage_c_sensor(sensor::Sensor *sensor) { this->ac_voltage_c_sensor_ = sensor; }
  void set_ac_current_a_sensor(sensor::Sensor *sensor) { this->ac_current_a_sensor_ = sensor; }
  void set_ac_current_b_sensor(sensor::Sensor *sensor) { this->ac_current_b_sensor_ = sensor; }
  void set_ac_current_c_sensor(sensor::Sensor *sensor) { this->ac_current_c_sensor_ = sensor; }
  void set_ac_current_total_sensor(sensor::Sensor *sensor) { this->ac_current_total_sensor_ = sensor; }
  void set_frequency_sensor(sensor::Sensor *sensor) { this->frequency_sensor_ = sensor; }
  void set_power_factor_sensor(sensor::Sensor *sensor) { this->power_factor_sensor_ = sensor; }
  void set_total_energy_sensor(sensor::Sensor *sensor) { this->total_energy_sensor_ = sensor; }
  void set_dc_voltage_sensor(sensor::Sensor *sensor) { this->dc_voltage_sensor_ = sensor; }
  void set_dc_current_sensor(sensor::Sensor *sensor) { this->dc_current_sensor_ = sensor; }
  void set_dc_power_sensor(sensor::Sensor *sensor) { this->dc_power_sensor_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }

 protected:
  // Modbus TCP server
  void start_server_();
  void handle_client_();
  void process_request_(WiFiClient &client, uint8_t *buffer, size_t len);
  void send_response_(WiFiClient &client, uint8_t *request, uint16_t start_addr, uint16_t reg_count);
  void send_error_(WiFiClient &client, uint8_t *request, uint8_t error_code);

  // SunSpec register management
  void init_registers_();
  void update_registers_();
  void write_string_(uint16_t offset, const char *str, uint16_t max_len);
  void write_uint32_(uint16_t offset, uint32_t value);

  // Simulation
  void update_simulation_();
  float calculate_power_();
  float add_noise_(float value, float max_noise);
  void publish_sensors_();

  // Configuration
  uint16_t port_{502};
  uint8_t unit_id_{1};
  std::string manufacturer_{"Growatt"};
  std::string model_{"9000 TL3-S"};
  std::string serial_{"EMULATED001"};
  uint32_t max_power_{9000};
  float grid_voltage_{230.0f};
  float grid_frequency_{50.0f};
  uint32_t update_interval_{1000};

  // Server state
  WiFiServer *server_{nullptr};
  WiFiClient client_;
  bool client_connected_{false};

  // SunSpec registers
  uint16_t registers_[TOTAL_REGISTERS];

  // Simulation state
  SimulatedValues values_;
  uint32_t start_time_{0};
  uint32_t last_update_{0};
  uint32_t accumulated_energy_{0};

  // Sensors
  sensor::Sensor *ac_power_sensor_{nullptr};
  sensor::Sensor *ac_voltage_a_sensor_{nullptr};
  sensor::Sensor *ac_voltage_b_sensor_{nullptr};
  sensor::Sensor *ac_voltage_c_sensor_{nullptr};
  sensor::Sensor *ac_current_a_sensor_{nullptr};
  sensor::Sensor *ac_current_b_sensor_{nullptr};
  sensor::Sensor *ac_current_c_sensor_{nullptr};
  sensor::Sensor *ac_current_total_sensor_{nullptr};
  sensor::Sensor *frequency_sensor_{nullptr};
  sensor::Sensor *power_factor_sensor_{nullptr};
  sensor::Sensor *total_energy_sensor_{nullptr};
  sensor::Sensor *dc_voltage_sensor_{nullptr};
  sensor::Sensor *dc_current_sensor_{nullptr};
  sensor::Sensor *dc_power_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
};

}  // namespace sunspec_modbus_server
}  // namespace esphome
