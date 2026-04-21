# Changelog

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
