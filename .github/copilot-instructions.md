# GitHub Copilot Instructions

## Repository Overview
This repository contains an ESPHome external component for controlling Midea HVAC systems over the XYE/CCM RS-485 bus. It provides a native Home Assistant climate entity with full mode, fan, and setpoint support.

## Development Guidelines

### Code Structure
- **Component Location**: All ESPHome components are in `esphome/components/`
- **Main Component**: `midea_xye` - The primary climate component for Midea HVAC control
- **Additional Components**: None currently

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
6. Set the PR title to `feat: <short description>` so release-please creates a minor-version release

### Fixing a Bug
1. Add or update tests to reproduce the issue
2. Implement the fix
3. Verify all tests still pass
4. Consider if documentation needs updates
5. Set the PR title to `fix: <short description>` so release-please creates a patch-version release

### Making Changes
- Keep changes minimal and focused
- Test changes locally before committing
- Ensure CI passes before requesting review
- Update tests to cover new functionality

### Conventional Commits (Required)

All PR titles and commit messages **must** follow the [Conventional Commits](https://www.conventionalcommits.org/) specification. release-please uses these to automatically determine the next version and generate changelogs.

| Prefix | When to use | Version bump |
|--------|-------------|--------------|
| `fix:` | Bug fix | patch (0.1.x) |
| `feat:` | New feature | minor (0.x.0) |
| `chore:` | Maintenance, CI, tooling — no release | none |
| `docs:` | Documentation only — no release | none |
| `refactor:` | Code restructure without behavior change — no release | none |
| `test:` | Adding or updating tests — no release | none |
| `ci:` | CI/CD changes — no release | none |
| `perf:` | Performance improvement | patch |

Examples:
- ✅ `fix: mask status flag bit from set temperature byte in C0 response`
- ✅ `feat: add outside temperature sensor for intelligent mode selection`
- ✅ `chore: update CI workflow for ESPHome compile checks`
- ❌ `Fix status flag in C0 response` (missing type prefix — release-please cannot parse this)

> **Important:** GitHub squash-merges use the PR title as the commit message. Always set the PR title to match the conventional commit format above.

## Architecture

### Component Structure
The component follows ESPHome's standard climate platform architecture:
- **Python Schema** (`climate.py`, `__init__.py`): Defines YAML configuration schema and validation
- **C++ Implementation** (`.cpp`, `.h` files): Core climate control logic and XYE protocol communication
- **Protocol**: XYE/CCM RS-485 bus protocol for Midea HVAC systems

### Key Files
- `air_conditioner.cpp/h`: Main AC control and state management
- `climate.py`: ESPHome climate platform configuration schema
- `ac_automations.h`: Automation helpers
- `ir_transmitter.h`: IR functionality (not yet fully implemented)
- `static_pressure_interface.h`: Pressure sensor interface
- `README.md`: Component-specific documentation

## Debugging

### Local Development
```bash
# Install dependencies
pip install -r requirements.txt

# Compile test configuration (validates component)
esphome compile tests/midea_xye.yaml

# For detailed compilation output
esphome -v compile tests/midea_xye.yaml
```

### Common Issues
- **UART conflicts**: Logger must have `baud_rate: 0` to avoid conflicts with UART used for RS-485
- **Compilation errors**: Usually indicate schema mismatches between Python and C++ code
- **Protocol debugging**: Enable UART debug in test YAML to see raw RS-485 traffic

### Component Dependencies
- ESPHome version specified in `requirements.txt`
- Standard ESPHome UART component for RS-485 communication
- ESPHome climate base class

## Important Constraints

### Protocol Limitations
- **XYE Protocol**: Communication is over RS-485 at 4800 baud
- **Follow-Me Feature**: Sends room temperature to AC unit (updates on change + every 30 seconds)
- **Known Limitations**: 
  - Current reading always returns 255 (hardware limitation)
  - Swing mode setting not working
  - Some features not yet implemented (timers, display unit forcing, silent mode, lock)

### Hardware Requirements
- ESP8266 or ESP32 board
- RS-485 to TTL converter
- Connection to Midea HVAC XYE/CCM bus

### Best Practices
- Always test compilation after schema changes
- Keep Python schema in sync with C++ implementation
- Follow ESPHome's climate platform conventions
- Document protocol reverse engineering discoveries in comments
