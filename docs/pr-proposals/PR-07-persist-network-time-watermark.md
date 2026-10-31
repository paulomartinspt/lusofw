# RFC: Persistent anti-replay state for network time synchronisation

## Problem

The timekeeper high-water mark is currently file-static RAM state. It resets after reboot, so a previously captured, validly signed timekeeper advert may be accepted again as the first advert after restart.

The trusted signature still authenticates the timekeeper; the issue is freshness state not surviving reboot.

## Proposed design

- Persist the highest accepted trusted timestamp (and, if needed, a format/version marker) using the project's existing preferences or durable storage layer.
- Define how first boot, missing/corrupt state, factory reset, and storage write failure are handled.
- Ensure the watermark is updated only after the RTC update and persistent write have succeeded according to a documented order.
- Avoid flash wear by coalescing writes or using a monotonic checkpoint policy while preserving replay guarantees.
- Do not use the local RTC as the sole anti-replay state: it may be unset, incorrect, or intentionally corrected backwards.
- Define an explicit recovery/reset process for trusted timekeeper key rotation.

## Acceptance criteria

- Replaying a previously accepted advert after reboot is rejected.
- A fresh signed advert advances the persisted high-water mark.
- A corrupted/missing persistence record fails safely and is observable.
- Flash write frequency is bounded and tested on all supported storage backends.
- Time correction policy and recovery behaviour are documented.

## Why this is an RFC

The project supports multiple storage platforms with different persistence and flash-wear characteristics. The durable store, failure semantics, and update frequency should be selected before changing the time-sync module.