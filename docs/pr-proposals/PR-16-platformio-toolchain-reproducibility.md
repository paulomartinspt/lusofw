# PlatformIO Core pinning and dependency-resolution follow-up

## Incremental hardening implemented

- The repository pins PlatformIO Core to `6.2.0` in the root `requirements-platformio.txt`.
- Main release, development prerelease and native test workflows install from that shared requirements file.
- Release workflows no longer run `pio pkg update` immediately before compiling. A release job should not deliberately refresh project packages as part of the release itself.

PlatformIO Core 6.2.0 was exercised by the native GitHub Actions matrix and the ESP32 OTA compile workflow before this pin was applied.

## Remaining reproducibility gaps

This is a first step, not a complete dependency lock. The project still contains semver ranges for several libraries and some PlatformIO platform/framework references that are not pinned to exact immutable versions. A fresh runner may still resolve a newer compatible package within those ranges.

The repository should not claim that the same source commit is guaranteed to produce byte-identical firmware until the supported board matrix has been built and a reviewed dependency manifest has been established.

## Follow-up plan

1. Inventory every PlatformIO platform, framework, toolchain, library and remote Git dependency used by the supported board matrix.
2. Select exact versions or immutable commit SHAs for each dependency, with explicit exceptions where upstream packages cannot be pinned safely.
3. Validate representative ESP32, NRF52, RP2040 and STM32 targets before changing the common dependency declarations.
4. Record the resolved dependency manifest and tool versions in CI artifacts and firmware release metadata.
5. Add a scheduled dependency-update workflow that proposes reviewed changes and runs the board matrix.

## Acceptance criteria for full reproducibility

- The same source commit uses the same PlatformIO Core, platforms, frameworks, toolchains and libraries.
- Release jobs do not upgrade dependencies immediately before compiling.
- Dependency updates arrive as reviewable changes and pass the supported target matrix.
- Build metadata records the toolchain and dependency versions actually used.
