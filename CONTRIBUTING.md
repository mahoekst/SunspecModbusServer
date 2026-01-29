# Contributing Guidelines

Thank you for your interest in contributing to the SunSpec Modbus Server project!

## How to Contribute

### Reporting Issues

Found a bug? Please open an issue with:

- Clear description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Device type and ESPHome version
- Relevant logs

### Suggesting Enhancements

Have a feature idea? Please open an issue with:

- Clear description of the feature
- Use case or problem it solves
- Proposed implementation (if applicable)

### Submitting Changes

1. **Fork the repository**
   ```bash
   git clone https://github.com/your-username/SunspecModbusServer.git
   ```

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the coding standards (see below)
   - Add tests if applicable
   - Update documentation

4. **Test thoroughly**
   ```bash
   esphome compile esphome/sunspec_server.yaml
   esphome run esphome/sunspec_server.yaml
   ```

5. **Commit with clear messages**
   ```bash
   git commit -m "Add feature: brief description"
   ```

6. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Open a Pull Request**
   - Describe the changes
   - Reference any related issues
   - Ensure CI/CD checks pass

## Coding Standards

### C++ Code

- Follow ESP-IDF conventions
- Use 2-space indentation
- Add `ESP_LOG*` macros for debugging
- Document complex algorithms
- Example:
  ```cpp
  void SunSpecModbus::update_simulated_values_() {
    // Update AC power with realistic fluctuation
    ac_power += (random(-5, 5) * 10.0f);
    this->write_register(40082, (int16_t)ac_power);
  }
  ```

### Python Code

- Follow PEP 8 style guide
- Use type hints
- Add docstrings
- Example:
  ```python
  def handle_register(address: int, value: int) -> bool:
      """Handle Modbus register write operation."""
      try:
          self.registers[address] = value
          return True
      except Exception as e:
          logger.error(f"Register write failed: {e}")
          return False
  ```

### YAML Configuration

- Use 2-space indentation
- Use consistent naming
- Add comments for clarity
- Example:
  ```yaml
  substitutions:
    device_name: sunspec-modbus
    friendly_name: "SunSpec Modbus Server"
  ```

## Documentation

When adding features, update documentation:

- [CONFIGURATION.md](docs/CONFIGURATION.md) - For configuration options
- [SUNSPEC_REGISTERS.md](docs/SUNSPEC_REGISTERS.md) - For new registers
- [DEVELOPMENT.md](docs/DEVELOPMENT.md) - For architectural changes
- Code comments - For implementation details

## Testing

Before submitting:

1. **Compile successfully**
   ```bash
   esphome compile esphome/sunspec_server.yaml
   ```

2. **Test functionality**
   - Read registers with Modbus client
   - Write registers
   - Verify simulated values update

3. **Check logs**
   ```bash
   esphome logs esphome/sunspec_server.yaml
   ```

## Code Review Process

1. Automated checks (CI/CD)
2. Maintainer review
3. Request for changes (if needed)
4. Approval and merge

## Development Workflow

```
Create Issue → Fork → Feature Branch → Code → Test → PR → Review → Merge
```

## Commit Message Format

```
[TYPE]: Brief description (50 chars max)

Longer explanation (wrap at 72 chars)

- Bullet points for changes
- Reference issues: Fixes #123
```

Types: `feat`, `fix`, `docs`, `style`, `test`, `refactor`

Example:
```
feat: Add Model 121 support

Implements SunSpec Model 121 (inverter extension) with:
- Additional voltage measurements
- Power factor reporting
- Temperature monitoring

Closes #45
```

## Questions?

- Check [DEVELOPMENT.md](docs/DEVELOPMENT.md)
- Open a discussion issue
- Contact maintainers

Thank you for contributing! 🎉
