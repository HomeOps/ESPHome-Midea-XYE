# Changelog

## [0.2.8](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.7...v0.2.8) (2026-05-28)


### Bug Fixes

* correct Fahrenheit setpoint reporting and C4 debug log ([#127](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/127)) ([6035f41](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/6035f4174ef5d06502d8e1985c6f7e2ca896b6b6))

## [0.2.7](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.6...v0.2.7) (2026-05-27)


### Bug Fixes

* report correct action states for FAN_ONLY and DRY modes ([#129](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/129)) ([f0a4fc2](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/f0a4fc2f2d5921650bd3e433f6ae9a04017f0d7e))

## [0.2.6](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.5...v0.2.6) (2026-05-23)


### Features

* sync HA fan mode from C4 target_fan_speed when sync_fan_mode_fr… ([#124](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/124)) ([9243612](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/9243612f27c9999b459d470184b009312bedcf83))

## [0.2.5](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.4...v0.2.5) (2026-05-18)


### Bug Fixes

* reclassify C4 byte 17 as target fan speed ([#122](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/122)) ([cbf0210](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/cbf0210a71231f6260331d70260aa4e7121a9bb6))

## [0.2.4](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.3...v0.2.4) (2026-05-18)


### Features

* compressor-aware climate action and diagnostic sensors ([#119](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/119)) ([e3cae9b](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/e3cae9b782a25a77ece362eeb130e678bcb49e7f))

## [0.2.3](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.2...v0.2.3) (2026-05-17)


### Bug Fixes

* increase temperature sensor display precision to 2 decimal places ([#115](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/115)) ([d1b169c](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/d1b169c56fea8e531dc8b491363317afc181fc0e))

## [0.2.2](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.1...v0.2.2) (2026-05-17)


### Bug Fixes

* reclassify C0 byte 19 as compressor-running flag and document bytes 28-29 steady-state values ([#112](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/112)) ([f47e54c](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/f47e54c243e40dd8026dafe571d88ba3473eaa42))

## [0.2.1](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.2.0...v0.2.1) (2026-04-21)


### Features

* add defrost active binary sensor ([#97](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/97)) ([#108](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/108)) ([0d67257](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/0d6725757fd2f0b6f7d8dbe7443f460021172771))

## [0.2.0](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.7...v0.2.0) (2026-04-21)


### ⚠ BREAKING CHANGES

* requires ESPHome >= 2026.4.0. Users pulling this as external_components with an older ESPHome will fail to compile. The deprecated traits setters would still work on 2026.4.0-2026.11.0, but we migrate now to keep the codebase on supported APIs.

### Features

* migrate to ESPHome 2026.4.0 custom-mode API ([#100](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/100)) ([#105](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/105)) ([515f786](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/515f78659662ef2e487e4b76a7c3198e16e95361))

## [0.1.7](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.6...v0.1.7) (2026-04-21)


### Bug Fixes

* suppress transient mode flap after SET ([#102](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/102)) ([#103](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/103)) ([fc56f99](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/fc56f997ea34fae086730083194bdb0a9b4e03e0))

## [0.1.6](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.5...v0.1.6) (2026-04-08)


### Bug Fixes

* correct CLIMATE_FAN_OFF mapping and response_timeout, add XYEAdapter, remove TXData/RXData legacy pointers ([#83](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/83)) ([4e9ae8c](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/4e9ae8cd8bfc7919c8119f3eb8e0120d38604855))

## [0.1.5](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.4...v0.1.5) (2026-04-05)


### Bug Fixes

* Mask 0x40 bit from set temperature byte in C0 response parsing ([#73](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/73)) ([f008a52](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/f008a52b70e4dfa0840ef6510eb2d10c5c46b319))

## [0.1.4](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.3...v0.1.4) (2026-04-05)


### Bug Fixes

* require conventional commits so release-please auto-generates releases ([#75](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/75)) ([ce1254f](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/ce1254f609ae86aeec27018b9bf3aeb7c2804617))

## [0.1.3](https://github.com/HomeOps/ESPHome-Midea-XYE/compare/v0.1.2...v0.1.3) (2026-04-05)


### Bug Fixes

* require conventional commits so release-please auto-generates releases ([#75](https://github.com/HomeOps/ESPHome-Midea-XYE/issues/75)) ([ce1254f](https://github.com/HomeOps/ESPHome-Midea-XYE/commit/ce1254f609ae86aeec27018b9bf3aeb7c2804617))
