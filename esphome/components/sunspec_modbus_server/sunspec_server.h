#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/number/number.h"

#ifdef USE_ARDUINO
#ifdef USE_ESP32
#include <WiFi.h>
#elif defined(USE_ESP8266)
#include <ESP8266WiFi.h>
#endif
#endif
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

// Model 120 (Nameplate Ratings) — inserted between Model 1 and Model 103
static const uint16_t MODEL120_ID_OFFSET = 69;
static const uint16_t MODEL120_LENGTH_OFFSET = 70;
static const uint16_t MODEL120_DATA_OFFSET = 71;
static const uint16_t MODEL120_LENGTH = 26;

static const uint16_t MODEL103_ID_OFFSET = 97;
static const uint16_t MODEL103_LENGTH_OFFSET = 98;
static const uint16_t MODEL103_DATA_OFFSET = 99;
static const uint16_t MODEL103_LENGTH = 50;

// Model 160 (Multiple MPPT) — 8 global + 2 trackers * 20 = 48 data registers
static const uint16_t MODEL160_ID_OFFSET = 149;
static const uint16_t MODEL160_LENGTH_OFFSET = 150;
static const uint16_t MODEL160_DATA_OFFSET = 151;
static const uint16_t MODEL160_LENGTH = 48;
static const uint16_t MODEL160_TRACKER_BASE = 8;    // data offset of first tracker block
static const uint16_t MODEL160_TRACKER_STRIDE = 20; // registers per tracker block

// Model 123 (Immediate Controls)
static const uint16_t MODEL123_ID_OFFSET = 199;
static const uint16_t MODEL123_LENGTH_OFFSET = 200;
static const uint16_t MODEL123_DATA_OFFSET = 201;
static const uint16_t MODEL123_LENGTH = 24;

// End marker and total
static const uint16_t END_MODEL_OFFSET = 225;
static const uint16_t TOTAL_REGISTERS = 227;

// Model 120 register offsets (relative to MODEL120_DATA_OFFSET)
namespace Model120 {
  static const uint8_t DERTyp = 0;       // DER type (4 = PV)
  static const uint8_t WRtg = 1;         // Continuous power rating (W)
  static const uint8_t WRtg_SF = 2;      // Scale factor
  static const uint8_t VARtg = 3;        // Continuous VA rating
  static const uint8_t VARtg_SF = 4;     // Scale factor
  static const uint8_t VArRtgQ1 = 5;     // VAr capability Q1
  static const uint8_t VArRtgQ2 = 6;     // VAr capability Q2
  static const uint8_t VArRtgQ3 = 7;     // VAr capability Q3
  static const uint8_t VArRtgQ4 = 8;     // VAr capability Q4
  static const uint8_t VArRtg_SF = 9;    // Scale factor
  static const uint8_t ARtg = 10;        // Max RMS AC current (sum of phases)
  static const uint8_t ARtg_SF = 11;     // Scale factor
  static const uint8_t PFRtgQ1 = 12;     // Min power factor Q1
  static const uint8_t PFRtgQ2 = 13;     // Min power factor Q2
  static const uint8_t PFRtgQ3 = 14;     // Min power factor Q3
  static const uint8_t PFRtgQ4 = 15;     // Min power factor Q4
  static const uint8_t PFRtg_SF = 16;    // Scale factor
  // 17-25: optional storage fields + pad (left as 0)
}  // namespace Model120

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

// Model 160 register offsets (relative to MODEL160_DATA_OFFSET)
namespace Model160 {
  static const uint8_t DCA_SF  = 0;   // Current scale factor (all trackers)
  static const uint8_t DCV_SF  = 1;   // Voltage scale factor
  static const uint8_t DCW_SF  = 2;   // Power scale factor
  static const uint8_t DCWH_SF = 3;   // Energy scale factor
  static const uint8_t Evt1    = 4;   // Global events (high word)
  static const uint8_t Evt2    = 5;   // Global events (low word)
  static const uint8_t N       = 6;   // Number of trackers
  static const uint8_t TmsPer  = 7;   // Timestamp period
  // Per-tracker block offsets (from start of tracker block, 20 regs each)
  static const uint8_t T_ID    = 0;   // Tracker input ID
  // T_IDStr occupies offsets 1-8 (8 registers)
  static const uint8_t T_DCA   = 9;   // DC current
  static const uint8_t T_DCV   = 10;  // DC voltage  ← Victron reads this
  static const uint8_t T_DCW   = 11;  // DC power    ← Victron reads this
}  // namespace Model160

// Model 123 register offsets (relative to MODEL123_DATA_OFFSET)
namespace Model123 {
  // Connection controls (3 registers before power limit fields)
  static const uint8_t Conn_WinTms = 0;        // Time to connect (s)
  static const uint8_t Conn_RvrtTms = 1;       // Time to revert connection (s)
  static const uint8_t Conn = 2;               // Connect/disconnect (1=connect)
  // Power limit controls — Victron writes at immediateControlOffset+5 = data[3]
  static const uint8_t WMaxLimPct = 3;         // Max power output %
  static const uint8_t WMaxLimPct_WinTms = 4;
  static const uint8_t WMaxLimPct_RvrtTms = 5;
  static const uint8_t WMaxLimPct_RmpTms = 6;
  static const uint8_t WMaxLim_Ena = 7;        // 0=disabled, 1=enabled
  static const uint8_t OutPFSet = 8;
  static const uint8_t OutPFSet_WinTms = 9;
  static const uint8_t OutPFSet_RvrtTms = 10;
  static const uint8_t OutPFSet_RmpTms = 11;
  static const uint8_t OutPFSet_Ena = 12;
  static const uint8_t VArPct_Mod = 13;
  static const uint8_t VArPct_WinTms = 14;
  static const uint8_t VArPct_RvrtTms = 15;
  static const uint8_t VArPct_RmpTms = 16;
  static const uint8_t VArSetPct = 17;
  static const uint8_t VArSetPct_Ena = 18;
  static const uint8_t WMaxLimPct_SF = 19;     // Scale factor (we set to 0)
  static const uint8_t OutPFSet_SF = 20;
  // 21-23: padding
}  // namespace Model123

// Inverter values (from source sensors)
struct InverterValues {
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
  void set_version(const std::string &version) { this->version_ = version; }
  void set_update_interval(uint32_t update_interval) { this->update_interval_ = update_interval; }
  void set_max_power(uint16_t max_power) { this->max_power_ = max_power; }

  // Source sensor setters (input from external components like modbus_controller)
  void set_source_ac_power(sensor::Sensor *sensor) { this->source_ac_power_ = sensor; }
  void set_source_voltage_a(sensor::Sensor *sensor) { this->source_voltage_a_ = sensor; }
  void set_source_voltage_b(sensor::Sensor *sensor) { this->source_voltage_b_ = sensor; }
  void set_source_voltage_c(sensor::Sensor *sensor) { this->source_voltage_c_ = sensor; }
  void set_source_current_a(sensor::Sensor *sensor) { this->source_current_a_ = sensor; }
  void set_source_current_b(sensor::Sensor *sensor) { this->source_current_b_ = sensor; }
  void set_source_current_c(sensor::Sensor *sensor) { this->source_current_c_ = sensor; }
  void set_source_frequency(sensor::Sensor *sensor) { this->source_frequency_ = sensor; }
  void set_source_power_factor(sensor::Sensor *sensor) { this->source_power_factor_ = sensor; }
  void set_source_total_energy(sensor::Sensor *sensor) { this->source_total_energy_ = sensor; }
  void set_source_dc_voltage(sensor::Sensor *sensor) { this->source_dc_voltage_ = sensor; }
  void set_source_dc_current(sensor::Sensor *sensor) { this->source_dc_current_ = sensor; }
  void set_source_dc_power(sensor::Sensor *sensor) { this->source_dc_power_ = sensor; }
  void set_source_temperature(sensor::Sensor *sensor) { this->source_temperature_ = sensor; }
  void set_source_pv2_voltage(sensor::Sensor *sensor) { this->source_pv2_voltage_ = sensor; }
  void set_source_pv2_current(sensor::Sensor *sensor) { this->source_pv2_current_ = sensor; }
  void set_source_pv2_power(sensor::Sensor *sensor) { this->source_pv2_power_ = sensor; }

  // Power limit number setter (target for Growatt active power rate)
  void set_power_limit_number(number::Number *number) { this->power_limit_number_ = number; }

  // Output sensor setters (publish to Home Assistant)
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
  void handle_write_single_(WiFiClient &client, uint8_t *buffer);
  void handle_write_multiple_(WiFiClient &client, uint8_t *buffer, size_t len);
  void process_write_(uint16_t reg_start, uint16_t reg_count);

  // SunSpec register management
  void init_registers_();
  void update_registers_();
  void write_string_(uint16_t offset, const char *str, uint16_t max_len);
  void write_uint32_(uint16_t offset, uint32_t value);

  // Data sources
  void update_from_sources_();
  void publish_sensors_();

  // Configuration
  uint16_t port_{502};
  uint8_t unit_id_{1};
  std::string manufacturer_{"Growatt"};
  std::string model_{"9000 TL3-S"};
  std::string serial_{"EMULATED001"};
  std::string version_{"1.0.0"};
  uint32_t update_interval_{1000};
  uint16_t max_power_{9000};

  // Server state
  WiFiServer *server_{nullptr};
  WiFiClient client_;
  bool client_connected_{false};
  uint32_t last_rx_ms_{0};
  static const uint32_t CLIENT_TIMEOUT_MS = 30000;  // 30 s without data → force disconnect

  // Revert timer: restores full power if Victron stops sending commands
  bool revert_active_{false};
  uint32_t revert_deadline_{0};

  // SunSpec registers
  uint16_t registers_[TOTAL_REGISTERS];

  // Inverter values
  InverterValues values_;
  uint32_t last_update_{0};

  // Output sensors (publish to Home Assistant)
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

  // Power limit number (target for Growatt active power rate)
  number::Number *power_limit_number_{nullptr};

  // Source sensors (input from external components like modbus_controller)
  sensor::Sensor *source_ac_power_{nullptr};
  sensor::Sensor *source_voltage_a_{nullptr};
  sensor::Sensor *source_voltage_b_{nullptr};
  sensor::Sensor *source_voltage_c_{nullptr};
  sensor::Sensor *source_current_a_{nullptr};
  sensor::Sensor *source_current_b_{nullptr};
  sensor::Sensor *source_current_c_{nullptr};
  sensor::Sensor *source_frequency_{nullptr};
  sensor::Sensor *source_power_factor_{nullptr};
  sensor::Sensor *source_total_energy_{nullptr};
  sensor::Sensor *source_dc_voltage_{nullptr};
  sensor::Sensor *source_dc_current_{nullptr};
  sensor::Sensor *source_dc_power_{nullptr};
  sensor::Sensor *source_temperature_{nullptr};
  sensor::Sensor *source_pv2_voltage_{nullptr};
  sensor::Sensor *source_pv2_current_{nullptr};
  sensor::Sensor *source_pv2_power_{nullptr};
};

}  // namespace sunspec_modbus_server
}  // namespace esphome
