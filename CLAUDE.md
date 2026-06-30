# CLAUDE.md

Guidance for AI coding agents (Claude, GitHub Copilot, and similar) working in this
repository. See [CONTRIBUTING.md](CONTRIBUTING.md) for the full contributor guide; the
rules below are mandatory for agent-generated changes.

## Pull requests must be narrow and single-purpose

**Every PR must address exactly one concern.** A PR that bundles unrelated changes — for
example a feature together with documentation rewrites, build/version-stamping tooling, or
unrelated field renames — **will be rejected and must be split**.

- One logical change per PR. If you need "and" to describe it, it is two PRs.
- Documentation-only changes go in their own `docs:` PR.
- Build/tooling/logging scaffolding that the feature does not require goes in its own PR.
- Wide renames or refactors go in their own PR, separate from behavioural changes.
- Prefer several small PRs over one large one, even when every part is individually useful.

## Conventional Commits (required)

PR titles and commit messages must follow [Conventional Commits](https://www.conventionalcommits.org/)
(`feat:`, `fix:`, `docs:`, `chore:`, `refactor:`, `test:`, `ci:`, `perf:`). `release-please`
uses them to determine the next version and generate the changelog. GitHub squash-merge uses
the PR title as the commit subject, so the title must match the format.

## Before opening a PR

- Keep the Python schema (`climate.py`) in sync with the C++ implementation; add schema
  validation for every new config key.
- Never use bare numeric literals for protocol values, byte offsets, or flag masks — define
  a named `constexpr` in `xye.h` (protocol-level) or `climate_midea_xye.h`
  (implementation-level), with a doc comment.
- Compile both test configurations:
  `esphome compile tests/midea_xye.yaml` and `esphome compile tests/midea_xye_esp32.yaml`.
- Exercise any new config key in at least one test YAML.
