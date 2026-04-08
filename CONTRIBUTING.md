# Contributing to ESPHome-Midea-XYE

Thank you for taking the time to contribute! This document explains how to set up your development environment, the coding conventions used in this project, and how to submit changes.

---

## Table of Contents

- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Project Structure](#project-structure)
- [Code Style](#code-style)
- [Testing](#testing)
- [Commit Messages](#commit-messages)
- [Submitting a Pull Request](#submitting-a-pull-request)
- [Reporting Issues](#reporting-issues)

---

## Getting Started

1. Fork the repository and clone your fork.
2. Create a new branch from `main` for your change:
   ```bash
   git checkout -b feat/my-new-feature
   ```
3. Make your changes, test them, and open a pull request.

---

## Development Setup

**Python 3.11** and the packages listed in `requirements.txt` are required.

```bash
pip install -r requirements.txt
```

This installs ESPHome and its dependencies so you can compile the test configurations locally.

---

## Project Structure

```
esphome/components/midea_xye/   # Component source (C++ + Python schema)
tests/                          # ESPHome YAML test configurations
.github/workflows/ci.yml        # CI pipeline
requirements.txt                # Python/ESPHome version pinning
```

Key files inside the component:

| File | Purpose |
|------|---------|
| `climate.py` | ESPHome YAML schema and validation |
| `air_conditioner.cpp/h` | Main AC state machine and XYE frame handling |
| `xye.h` | XYE protocol constants and frame definitions |
| `climate_midea_xye.h/cpp` | ESPHome climate platform glue |
| `PROTOCOL.md` | Reverse-engineered protocol documentation |

---

## Code Style

### C++

- Follow the ESPHome C++ coding conventions (2-space indentation, `snake_case` names).
- **Never use bare numeric literals** for protocol values, byte offsets, flag masks, or field sizes.  
  Always define a named `constexpr` constant with a descriptive name and a doc comment:
  - Protocol-level values → `xye.h`
  - Implementation-level constants → `climate_midea_xye.h`
- Use the ESPHome log macros at the appropriate level:
  - `ESP_LOGD` – debug / protocol tracing
  - `ESP_LOGI` – informational lifecycle events
  - `ESP_LOGW` – recoverable warnings
  - `ESP_LOGE` – errors

### YAML / Python schema

- Use 2-space indentation in YAML files.
- Keep the Python schema (`climate.py`) in sync with the C++ implementation.
- Add schema validation for every new config key.

---

## Testing

CI compiles two ESPHome configurations to verify the component builds without errors:

| Config | Target |
|--------|--------|
| `tests/midea_xye.yaml` | ESP8266 |
| `tests/midea_xye_esp32.yaml` | ESP32 |

Run both locally before opening a PR:

```bash
esphome compile tests/midea_xye.yaml
esphome compile tests/midea_xye_esp32.yaml
```

For verbose output:

```bash
esphome -v compile tests/midea_xye.yaml
```

Any new configuration key or feature should be exercised in at least one of the test YAML files.

---

## Commit Messages

All commit messages and **PR titles** must follow the [Conventional Commits](https://www.conventionalcommits.org/) specification. `release-please` uses these to determine the next version number and generate the changelog automatically.

| Prefix | When to use | Version bump |
|--------|-------------|--------------|
| `fix:` | Bug fix | patch (`0.1.x`) |
| `feat:` | New feature | minor (`0.x.0`) |
| `chore:` | Maintenance, CI, tooling — no release | none |
| `docs:` | Documentation only — no release | none |
| `refactor:` | Code restructure without behaviour change — no release | none |
| `test:` | Adding or updating tests — no release | none |
| `ci:` | CI/CD changes — no release | none |
| `perf:` | Performance improvement | patch |

Examples:

```
fix: mask status flag bit from set temperature byte in C0 response
feat: add outside temperature sensor for intelligent mode selection
chore: update CI workflow for ESPHome compile checks
```

> **Important:** GitHub squash-merges use the PR title as the commit message.  
> Always set the PR title to match the conventional commit format above.

---

## Submitting a Pull Request

1. Ensure both test configurations compile successfully.
2. Update `README.md` or `PROTOCOL.md` if your change affects documented behaviour.
3. Set the PR title using the conventional commit format (see above).
4. Describe **what** changed and **why** in the PR description.
5. Reference any related issues with `Closes #<issue>` or `Fixes #<issue>`.

---

## Reporting Issues

Please open a [GitHub Issue](https://github.com/HomeOps/ESPHome-Midea-XYE/issues) and include:

- Your ESPHome version (`esphome version`)
- Your hardware (ESP8266 / ESP32 board model)
- Relevant YAML configuration (redact any secrets)
- Debug logs with `midea_xye: DEBUG` enabled (see `README.md` for instructions)
