import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_PORT,
    CONF_UPDATE_INTERVAL,
    UNIT_WATT,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_HERTZ,
    UNIT_CELSIUS,
    UNIT_WATT_HOURS,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_FREQUENCY,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_ENERGY,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
)
from esphome.components import sensor

CODEOWNERS = ["@mahoekst"]
DEPENDENCIES = ["network"]
AUTO_LOAD = ["sensor"]

CONF_UNIT_ID = "unit_id"
CONF_MANUFACTURER = "manufacturer"
CONF_MODEL = "model"
CONF_SERIAL = "serial"

# Source sensor configuration keys (input from external sensors like modbus_controller)
CONF_SOURCE_AC_POWER = "source_ac_power"
CONF_SOURCE_VOLTAGE_A = "source_voltage_a"
CONF_SOURCE_VOLTAGE_B = "source_voltage_b"
CONF_SOURCE_VOLTAGE_C = "source_voltage_c"
CONF_SOURCE_CURRENT_A = "source_current_a"
CONF_SOURCE_CURRENT_B = "source_current_b"
CONF_SOURCE_CURRENT_C = "source_current_c"
CONF_SOURCE_FREQUENCY = "source_frequency"
CONF_SOURCE_POWER_FACTOR = "source_power_factor"
CONF_SOURCE_TOTAL_ENERGY = "source_total_energy"
CONF_SOURCE_DC_VOLTAGE = "source_dc_voltage"
CONF_SOURCE_DC_CURRENT = "source_dc_current"
CONF_SOURCE_DC_POWER = "source_dc_power"
CONF_SOURCE_TEMPERATURE = "source_temperature"

# Output sensor configuration keys (publish to Home Assistant)
CONF_AC_POWER = "ac_power"
CONF_AC_VOLTAGE_A = "ac_voltage_a"
CONF_AC_VOLTAGE_B = "ac_voltage_b"
CONF_AC_VOLTAGE_C = "ac_voltage_c"
CONF_AC_CURRENT_A = "ac_current_a"
CONF_AC_CURRENT_B = "ac_current_b"
CONF_AC_CURRENT_C = "ac_current_c"
CONF_AC_CURRENT_TOTAL = "ac_current_total"
CONF_FREQUENCY = "frequency"
CONF_POWER_FACTOR = "power_factor"
CONF_TOTAL_ENERGY = "total_energy"
CONF_DC_VOLTAGE = "dc_voltage"
CONF_DC_CURRENT = "dc_current"
CONF_DC_POWER = "dc_power"
CONF_TEMPERATURE = "temperature"

sunspec_modbus_server_ns = cg.esphome_ns.namespace("sunspec_modbus_server")
SunSpecModbusServer = sunspec_modbus_server_ns.class_("SunSpecModbusServer", cg.Component)

SENSOR_SCHEMA = sensor.sensor_schema()

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SunSpecModbusServer),
        cv.Optional(CONF_PORT, default=502): cv.port,
        cv.Optional(CONF_UNIT_ID, default=1): cv.int_range(min=1, max=247),
        cv.Optional(CONF_MANUFACTURER, default="Growatt"): cv.string,
        cv.Optional(CONF_MODEL, default="9000 TL3-S"): cv.string,
        cv.Optional(CONF_SERIAL, default="EMULATED001"): cv.string,
        cv.Optional(CONF_UPDATE_INTERVAL, default="1s"): cv.update_interval,
        # Source sensors (input from external components like modbus_controller)
        cv.Optional(CONF_SOURCE_AC_POWER): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_VOLTAGE_A): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_VOLTAGE_B): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_VOLTAGE_C): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_CURRENT_A): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_CURRENT_B): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_CURRENT_C): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_FREQUENCY): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_POWER_FACTOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_TOTAL_ENERGY): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_DC_VOLTAGE): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_DC_CURRENT): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_DC_POWER): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SOURCE_TEMPERATURE): cv.use_id(sensor.Sensor),
        # Output sensor configurations (publish to Home Assistant)
        cv.Optional(CONF_AC_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_VOLTAGE_A): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_VOLTAGE_B): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_VOLTAGE_C): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_CURRENT_A): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_CURRENT_B): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_CURRENT_C): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_AC_CURRENT_TOTAL): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_FREQUENCY): sensor.sensor_schema(
            unit_of_measurement=UNIT_HERTZ,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_FREQUENCY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_POWER_FACTOR): sensor.sensor_schema(
            accuracy_decimals=2,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TOTAL_ENERGY): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional(CONF_DC_VOLTAGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DC_CURRENT): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_DC_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_unit_id(config[CONF_UNIT_ID]))
    cg.add(var.set_manufacturer(config[CONF_MANUFACTURER]))
    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_serial(config[CONF_SERIAL]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))

    # Register source sensors (input from external components)
    if CONF_SOURCE_AC_POWER in config:
        sens = await cg.get_variable(config[CONF_SOURCE_AC_POWER])
        cg.add(var.set_source_ac_power(sens))

    if CONF_SOURCE_VOLTAGE_A in config:
        sens = await cg.get_variable(config[CONF_SOURCE_VOLTAGE_A])
        cg.add(var.set_source_voltage_a(sens))

    if CONF_SOURCE_VOLTAGE_B in config:
        sens = await cg.get_variable(config[CONF_SOURCE_VOLTAGE_B])
        cg.add(var.set_source_voltage_b(sens))

    if CONF_SOURCE_VOLTAGE_C in config:
        sens = await cg.get_variable(config[CONF_SOURCE_VOLTAGE_C])
        cg.add(var.set_source_voltage_c(sens))

    if CONF_SOURCE_CURRENT_A in config:
        sens = await cg.get_variable(config[CONF_SOURCE_CURRENT_A])
        cg.add(var.set_source_current_a(sens))

    if CONF_SOURCE_CURRENT_B in config:
        sens = await cg.get_variable(config[CONF_SOURCE_CURRENT_B])
        cg.add(var.set_source_current_b(sens))

    if CONF_SOURCE_CURRENT_C in config:
        sens = await cg.get_variable(config[CONF_SOURCE_CURRENT_C])
        cg.add(var.set_source_current_c(sens))

    if CONF_SOURCE_FREQUENCY in config:
        sens = await cg.get_variable(config[CONF_SOURCE_FREQUENCY])
        cg.add(var.set_source_frequency(sens))

    if CONF_SOURCE_POWER_FACTOR in config:
        sens = await cg.get_variable(config[CONF_SOURCE_POWER_FACTOR])
        cg.add(var.set_source_power_factor(sens))

    if CONF_SOURCE_TOTAL_ENERGY in config:
        sens = await cg.get_variable(config[CONF_SOURCE_TOTAL_ENERGY])
        cg.add(var.set_source_total_energy(sens))

    if CONF_SOURCE_DC_VOLTAGE in config:
        sens = await cg.get_variable(config[CONF_SOURCE_DC_VOLTAGE])
        cg.add(var.set_source_dc_voltage(sens))

    if CONF_SOURCE_DC_CURRENT in config:
        sens = await cg.get_variable(config[CONF_SOURCE_DC_CURRENT])
        cg.add(var.set_source_dc_current(sens))

    if CONF_SOURCE_DC_POWER in config:
        sens = await cg.get_variable(config[CONF_SOURCE_DC_POWER])
        cg.add(var.set_source_dc_power(sens))

    if CONF_SOURCE_TEMPERATURE in config:
        sens = await cg.get_variable(config[CONF_SOURCE_TEMPERATURE])
        cg.add(var.set_source_temperature(sens))

    # Register output sensors (publish to Home Assistant)
    if CONF_AC_POWER in config:
        sens = await sensor.new_sensor(config[CONF_AC_POWER])
        cg.add(var.set_ac_power_sensor(sens))

    if CONF_AC_VOLTAGE_A in config:
        sens = await sensor.new_sensor(config[CONF_AC_VOLTAGE_A])
        cg.add(var.set_ac_voltage_a_sensor(sens))

    if CONF_AC_VOLTAGE_B in config:
        sens = await sensor.new_sensor(config[CONF_AC_VOLTAGE_B])
        cg.add(var.set_ac_voltage_b_sensor(sens))

    if CONF_AC_VOLTAGE_C in config:
        sens = await sensor.new_sensor(config[CONF_AC_VOLTAGE_C])
        cg.add(var.set_ac_voltage_c_sensor(sens))

    if CONF_AC_CURRENT_A in config:
        sens = await sensor.new_sensor(config[CONF_AC_CURRENT_A])
        cg.add(var.set_ac_current_a_sensor(sens))

    if CONF_AC_CURRENT_B in config:
        sens = await sensor.new_sensor(config[CONF_AC_CURRENT_B])
        cg.add(var.set_ac_current_b_sensor(sens))

    if CONF_AC_CURRENT_C in config:
        sens = await sensor.new_sensor(config[CONF_AC_CURRENT_C])
        cg.add(var.set_ac_current_c_sensor(sens))

    if CONF_AC_CURRENT_TOTAL in config:
        sens = await sensor.new_sensor(config[CONF_AC_CURRENT_TOTAL])
        cg.add(var.set_ac_current_total_sensor(sens))

    if CONF_FREQUENCY in config:
        sens = await sensor.new_sensor(config[CONF_FREQUENCY])
        cg.add(var.set_frequency_sensor(sens))

    if CONF_POWER_FACTOR in config:
        sens = await sensor.new_sensor(config[CONF_POWER_FACTOR])
        cg.add(var.set_power_factor_sensor(sens))

    if CONF_TOTAL_ENERGY in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_ENERGY])
        cg.add(var.set_total_energy_sensor(sens))

    if CONF_DC_VOLTAGE in config:
        sens = await sensor.new_sensor(config[CONF_DC_VOLTAGE])
        cg.add(var.set_dc_voltage_sensor(sens))

    if CONF_DC_CURRENT in config:
        sens = await sensor.new_sensor(config[CONF_DC_CURRENT])
        cg.add(var.set_dc_current_sensor(sens))

    if CONF_DC_POWER in config:
        sens = await sensor.new_sensor(config[CONF_DC_POWER])
        cg.add(var.set_dc_power_sensor(sens))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
