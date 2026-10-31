# RFC: Reproducible PlatformIO toolchain and dependency resolution

## Problem

The release workflows install the latest PlatformIO CLI and run `pio pkg update` before building. The project also contains version ranges for several libraries. This means the same source commit can resolve different build tools or package versions at different times.

## Proposed design

1. Select and document the supported PlatformIO Core version.
2. Pin framework/platform packages and libraries to reviewed exact versions or immutable commit SHAs where appropriate.
3. Remove automatic package upgrades from release jobs; release builds should consume the reviewed dependency set.
4. Introduce a separate, scheduled or manually triggered dependency-update workflow that opens reviewed pull requests.
5. Record the toolchain version and resolved dependency manifest with release artifacts.
6. Test the complete supported firmware matrix before changing the lock set.

## Acceptance criteria

- Rebuilding the same source commit uses the same PlatformIO Core, platforms, frameworks, and libraries.
- Release jobs never silently upgrade dependencies immediately before compiling.
- Dependency updates are delivered as reviewable changes and tested across the supported target matrix.
- Build metadata records the toolchain and dependency versions used.

## Why this is an RFC

Pinning every dependency without first checking the supported board matrix can break older targets or conflict with upstream forks. The exact versions and update policy should be agreed and tested before replacing the current ranges.