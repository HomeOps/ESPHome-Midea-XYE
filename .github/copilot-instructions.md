# GitHub Copilot Instructions

## Repository Overview
This repository contains an ESPHome external component for controlling Midea HVAC systems over the XYE/CCM RS-485 bus. It provides a native Home Assistant climate entity with full mode, fan, and setpoint support.

## Development Guidelines

### Code Structure
- **Component Location**: All ESPHome components are in `esphome/components/`
- **Main Component**: `midea_xye` - The primary climate component for Midea HVAC control
- **Additional Components**: 
  - `virtual_thermostat` - Virtual thermostat component for testing

### Testing
- Test configurations are located in the `tests/` directory
- Main test file: `tests/midea_xye.yaml`
- Tests should compile successfully with ESPHome
- CI pipeline automatically tests all configurations on push/PR

### Building and Testing
- **Build Command**: `esphome compile tests/midea_xye.yaml`
- **Dependencies**: Listed in `requirements.txt`
- **Python Version**: 3.11
- **ESPHome Version**: Check `requirements.txt` for current version

### Component Development
- Follow ESPHome component development patterns
- Components must include proper YAML schema definitions
- C++ components should have corresponding Python schema in `__init__.py` or `climate.py`
- Use proper logging levels (ESP_LOGD, ESP_LOGI, ESP_LOGW, ESP_LOGE)

### CI/CD
- GitHub Actions workflow in `.github/workflows/ci.yml`
- Automatically runs on push to `main` and `develop` branches
- Compiles all test configurations to verify component functionality
- Must pass before merging PRs

### Code Style
- Follow ESPHome C++ coding conventions
- Use 2-space indentation for YAML files
- Keep code modular and well-documented
- Add comments for complex logic

### External Components
- Components are designed to be used as external components in ESPHome configurations
- Users reference them via the `external_components` directive pointing to this repository

## Common Tasks

### Adding a New Feature
1. Implement the feature in the relevant C++ files
2. Update the Python schema if needed
3. Add test coverage in `tests/midea_xye.yaml`
4. Verify compilation with `esphome compile tests/midea_xye.yaml`
5. Update documentation if needed

### Fixing a Bug
1. Add or update tests to reproduce the issue
2. Implement the fix
3. Verify all tests still pass
4. Consider if documentation needs updates

### Making Changes
- Keep changes minimal and focused
- Test changes locally before committing
- Ensure CI passes before requesting review
- Update tests to cover new functionality
