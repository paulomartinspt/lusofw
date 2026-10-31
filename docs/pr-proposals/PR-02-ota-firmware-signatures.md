# RFC: Cryptographic verification of OTA firmware images

## Problem

The current OTA flow checks an MD5 value supplied with the upload. MD5 can detect accidental corruption, but it does not authenticate the firmware publisher: an attacker able to reach the OTA endpoint can replace both the image and its MD5 value.

## Proposed design

1. Define a signed firmware manifest containing target board/platform, firmware version, image size, image digest, and signing-key identifier.
2. Sign the manifest offline with a dedicated release-signing key.
3. Embed only the corresponding public verification key in firmware.
4. Verify the signature, target compatibility, image size, and digest before committing the update.
5. Define key rotation and recovery procedures before enforcing verification in released builds.
6. Keep development/test signing keys separate from production release keys.

## Compatibility and rollout

- Decide how unsigned legacy images are handled during migration.
- Ensure each supported ESP32 target uses a verification path compatible with its bootloader and flash layout.
- Consider secure boot and anti-rollback support where hardware permits.
- Define recovery when a signing key is revoked.

## Acceptance criteria

- Modified images/manifests are rejected.
- Images signed by an untrusted key are rejected.
- Images for a different target are rejected.
- Power loss during validation/update leaves a bootable image.
- Private signing keys are never exposed to CI build jobs.

## Why this is an RFC

This affects release infrastructure, bootloader compatibility, key custody, and recovery. It should not be implemented as a local MD5 replacement without an end-to-end signing and verification design.
