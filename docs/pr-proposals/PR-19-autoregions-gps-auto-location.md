# RFC: Use available GPS fixes for automatic AutoRegions assignment

**Status:** Proposal / feature suggestion  
**Scope:** Repeater firmware and GPS-capable device variants  
**Implementation:** Not included in this PR

## Summary

Allow `AutoRegions` to use a device's live GPS position when the selected firmware target has a GPS provider and the device can obtain a valid fix. The coordinates should flow directly from the existing location-provider subsystem into the existing region-polygon evaluation. Users should no longer have to run `gps setloc` just to make automatic region assignment use the GPS position.

The existing country, macro-region, district, Europe and RF-scope definitions remain the source of truth. This proposal is about supplying a reliable coordinate source and triggering re-evaluation at the right time; it does not propose a new region database or a new polygon algorithm.

This is a documentation-only suggestion. The implementation details and power/privacy policy should be agreed before a separate code PR is opened.

## Current implementation

The repository already contains most of the GPS building blocks:

- `LocationProvider` defines the common interface for latitude, longitude, validity, timestamps, satellite count, start/stop and polling.
- `MicroNMEALocationProvider` implements that interface for serial NMEA receivers. Its latitude and longitude values are exposed in microdegrees.
- `EnvironmentSensorManager` can compile in GPS support, detect a receiver, start and stop it, poll it, and copy a valid fix into `SensorManager::node_lat` / `node_lon` as decimal degrees.
- `SensorManager::isGPSDetected()` and `getLocationProvider()` already expose the runtime detection/provider state.
- The RAK WisBlock path also has a board-specific GPS provider.

However, the current AutoRegions path does not consume that live position:

1. `AutoRegions::checkRegionAutoAssign()` reads `NodePrefs::node_lat` and `node_lon`. It does not query `SensorManager` or a `LocationProvider`.
2. `EnvironmentSensorManager::loop()` updates the live sensor coordinates, but it does not notify AutoRegions when a new fix arrives or changes.
3. The existing `gps setloc` CLI command manually copies `sensors.node_lat` / `node_lon` into `NodePrefs`, saves the preferences, and invokes `onNodeConfigChanged()`. This is currently the bridge between live GPS and the AutoRegions evaluation callback.
4. The repeater's GPS preference defaults to disabled (`gps_enabled = 0`). The initial AutoRegions evaluation happens during `MyMesh::begin()` before `applyGpsPrefs()` is called.
5. GPS support is compile-time gated by `ENV_INCLUDE_GPS`. A board having GPS pins is not, by itself, enough to create a provider. For example, the Heltec T114 target has GPS pins and a provider in `target.cpp`, while the current `ENV_INCLUDE_GPS=1` flag is set for its companion-radio environment rather than its repeater environment. The LilyGo T-Beam SX1262 profile currently leaves its GPS enable flag commented out.
6. AutoRegions currently treats configured coordinates as available only when they are non-zero, valid and `advert_loc_policy != ADVERT_LOC_NONE`. If they are unavailable, it tries the node-name prefix fallback. Compile-time `ADVERT_LAT` / `ADVERT_LON` defaults are used for radio regulation only; they do not create geographical regions.

Relevant code:

- [AutoRegions.cpp](../../src/lusofw/AutoRegions.cpp) and [AutoRegions.h](../../src/lusofw/AutoRegions.h)
- [SensorManager.h](../../src/helpers/SensorManager.h)
- [LocationProvider.h](../../src/helpers/sensors/LocationProvider.h) and [MicroNMEALocationProvider.h](../../src/helpers/sensors/MicroNMEALocationProvider.h)
- [EnvironmentSensorManager.h](../../src/helpers/sensors/EnvironmentSensorManager.h) and [EnvironmentSensorManager.cpp](../../src/helpers/sensors/EnvironmentSensorManager.cpp)
- [MyMesh.cpp](../../examples/simple_repeater/MyMesh.cpp) and [MyMesh.h](../../examples/simple_repeater/MyMesh.h)
- [CommonCLI.cpp](../../src/helpers/CommonCLI.cpp), including the existing `gps setloc` command and advert-location policy
- [Heltec T114 target](../../variants/heltec_t114/target.cpp) and [build profiles](../../variants/heltec_t114/platformio.ini)
- [LilyGo T-Beam SX1262 build profile](../../variants/lilygo_tbeam_SX1262/platformio.ini)

## Proposed behaviour

### 1. Use the existing GPS provider, not a second GPS implementation

Keep hardware-specific detection and power management in `EnvironmentSensorManager` and the existing `LocationProvider` implementations. AutoRegions should receive a selected coordinate pair (and, ideally, its source/freshness metadata) rather than knowing about UARTs, GPIO pins, MicroNMEA, RAK GPS hardware or individual boards.

Only targets that explicitly compile GPS support and have a detected/usable provider should use this path. Do not infer GPS availability from pin definitions alone.

### 2. Acquire a fix automatically on GPS-capable devices

When a GPS-capable target detects a receiver, AutoRegions should be able to obtain a fix without requiring the user to run `gps setloc`.

Recommended power policy:

- If the existing GPS preference is enabled, use the live valid fixes already being collected and re-evaluate when the position changes.
- If continuous GPS is disabled, allow a bounded, one-shot acquisition for AutoRegions: power the receiver as required, wait for a valid fresh fix up to a documented timeout, evaluate the regions, then release/stop the receiver.
- Do not leave a receiver powered continuously solely because AutoRegions needs one location fix.
- If acquisition times out, the receiver is not detected, or the fix is invalid/stale, fall back without blocking normal mesh startup.

The exact timeout and whether one-shot acquisition should be controlled by a dedicated preference should be decided during implementation. The default experience on a GPS-equipped repeater should be automatic, while still giving users a clear way to disable GPS use for regional inference.

### 3. Reuse the existing region catalog and assignment rules

Once a valid GPS fix is available, pass its latitude/longitude to the existing polygon evaluation. Reuse the compiled-in country registry and all existing region layers:

- country membership;
- macro-regions;
- districts;
- `#europe` membership;
- country-specific RF scopes such as Portugal's `#pt-433` and `#pt-868`;
- existing hierarchy injection, cycle checks and region-map persistence.

Do not duplicate polygon definitions or introduce a second region-assignment implementation for GPS.

### 4. Define coordinate-source precedence explicitly

A suggested resolution order is:

1. A valid, fresh GPS fix when automatic GPS region assignment is enabled.
2. Manually configured `NodePrefs::node_lat` / `node_lon` when no usable GPS fix is available.
3. The existing node-name prefix fallback when neither coordinate source is usable.
4. Compile-time default coordinates remain regulation-only and must not create regions.

Reviewers should confirm how an explicit manual coordinate override should interact with a live GPS fix. If manual coordinates must remain authoritative, expose that distinction explicitly instead of silently selecting a different source.

### 5. Keep live GPS location separate from advertised/persisted exact coordinates

Do not copy every live GPS fix into `NodePrefs::node_lat` / `node_lon` or persist exact GPS coordinates as a side effect of AutoRegions. The existing `gps setloc` command should retain its explicit/manual-copy meaning.

The existing `advert_loc_policy` must continue to control whether exact coordinates are included in advertisements. GPS-based region assignment is local processing, but the resulting coarse region membership may itself reveal approximate location. The implementation should document that privacy implication and provide a clear opt-out for GPS-based AutoRegions. Review whether this should be a separate preference or whether `ADVERT_LOC_NONE` should also disable local GPS inference; do not change the current advert policy semantics silently.

### 6. Re-evaluate only when useful

- Trigger an evaluation when the first valid GPS fix becomes available.
- For a continuously active GPS, re-evaluate after the position has moved beyond a defined threshold or crossed a relevant region boundary.
- Use a freshness check and input validation (`isValid()`, finite coordinates, latitude [-90, 90], longitude [-180, 180]).
- Avoid repeated `RegionMap` writes when the resulting region tree has not changed. Preserve the current map-change-only persistence behaviour.
- Avoid rapid oscillation near polygon borders; define a stability/hysteresis policy if GPS jitter can repeatedly switch between adjacent polygons.

## Fallback and failure behaviour

| GPS / coordinate state | Expected AutoRegions behaviour |
|---|---|
| GPS support not compiled for the target | Keep current configured-coordinate and name-prefix behaviour |
| No GPS provider detected | Use configured coordinates, then name-prefix fallback |
| GPS detected but no valid fix before timeout | Use configured coordinates, then name-prefix fallback |
| GPS fix invalid, stale or out of range | Ignore that fix and use the existing fallback chain |
| Valid GPS fix available | Use the selected GPS coordinates for the existing polygon evaluation |
| GPS later stops or loses fix | Do not replace a previously valid region assignment with zero/invalid coordinates; retain the last safe result or fall back according to the agreed policy |

A missing GPS receiver or a slow time-to-first-fix must never hold up the repeater's normal mesh/radio startup.

## Acceptance criteria for an implementation PR

- [ ] GPS-capable repeater build profiles explicitly enable the existing provider where the hardware is present; non-GPS profiles remain unchanged.
- [ ] A detected GPS receiver can provide the first AutoRegions location automatically, without `gps setloc`.
- [ ] The implementation does not require continuous GPS operation just to obtain a one-time region assignment.
- [ ] A valid GPS fix is converted to decimal degrees correctly and validated before polygon lookup.
- [ ] The current country, macro-region, district, Europe and RF-region catalog is reused unchanged.
- [ ] GPS location updates trigger AutoRegions re-evaluation without requiring a CLI/configuration change.
- [ ] Manual configured coordinates and node-name fallback remain available when GPS is unavailable.
- [ ] Exact GPS coordinates are not silently copied into persistent node preferences.
- [ ] `advert_loc_policy` behaviour for advertisements remains unchanged and the coarse-location privacy choice is documented.
- [ ] No-GPS builds continue to compile and behave as before.
- [ ] Region-map flash writes occur only when the resulting map changes.
- [ ] Unit tests cover GPS success, no provider, no fix/timeout, invalid coordinates, stale fix, manual-coordinate fallback, name-prefix fallback, movement across a region boundary and repeated identical fixes.
- [ ] Hardware tests cover at least one serial GPS target (for example, Heltec T114) and one supported alternative provider/board if included in the build matrix.

## Suggested implementation split

1. **GPS integration:** expose a small, board-independent location-acquisition/update API through `SensorManager` / `EnvironmentSensorManager`.
2. **AutoRegions input:** decouple polygon evaluation from `NodePrefs` so it can evaluate an explicit GPS coordinate pair without persisting the exact position.
3. **Lifecycle and scheduling:** acquire a one-shot fix when appropriate, reuse continuous fixes when GPS is already enabled, and trigger region re-evaluation with rate limiting.
4. **Target enablement:** enable `ENV_INCLUDE_GPS` only in repeater profiles whose hardware actually includes a GPS receiver.
5. **Tests and hardware validation:** add native tests for source selection and fallback behaviour, then verify acquisition/power lifecycle on real GPS boards.

## Open design questions

1. Should GPS auto-region acquisition run by default on every GPS-capable repeater, or should a dedicated `auto_regions_gps` preference control it?
2. Should an explicit manual `set lat` / `set lon` override a live GPS fix, or should GPS take precedence while a valid fix exists?
3. Should `ADVERT_LOC_NONE` suppress only exact-coordinate advertisements, or also GPS-derived coarse region assignment? A separate opt-out avoids coupling two different behaviours.
4. What is the acceptable one-shot fix timeout and power budget for each GPS-equipped board?
5. What freshness/hysteresis rule should be used when a GPS provider reports a valid but old or jittering fix?

## Scope of this PR

This PR records the feature suggestion and proposed acceptance criteria only. It does not modify firmware behaviour, enable GPS flags on any board, or change the current AutoRegions policy. Implementation should follow after the power, source-precedence and privacy questions above have been reviewed.
