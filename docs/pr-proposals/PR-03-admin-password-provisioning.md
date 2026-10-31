# RFC: Secure first-boot administration credential provisioning

## Problem

The repeater example provides a fallback admin password of `password`, and several Heltec repeater environments set the same value explicitly. A predictable credential is unsafe when devices are deployed without an owner changing it.

## Proposed design

- Remove the universal production default.
- Create a per-device credential using the hardware random source during first-boot provisioning.
- Display or export the initial credential only through a documented local, owner-controlled provisioning path.
- Require the owner to change or explicitly acknowledge the credential before enabling remote administration.
- Preserve an authenticated recovery/reset procedure that does not create a universal backdoor.
- Keep development defaults isolated from release builds.

## Migration considerations

- Specify behaviour for existing devices with stored preferences; do not silently overwrite configured credentials.
- Define factory-reset behaviour.
- Ensure all supported boards have a provisioning path, including headless repeaters.
- Document guest-password behaviour separately from administrator authentication.

## Acceptance criteria

- No production build ships with a known universal admin password.
- Fresh devices cannot be remotely administered with a published default credential.
- Existing configured passwords survive firmware upgrades.
- Factory reset does not restore a universal shared password.
- Provisioning and recovery are documented and tested on supported target families.

## Why this is an RFC

Removing a compile-time default alone could lock out headless devices or change upgrade behaviour. Provisioning and recovery must be agreed before enforcement.