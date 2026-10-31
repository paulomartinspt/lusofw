# RFC: Authenticated encryption for the ESP-NOW bridge

## Problem

The current ESP-NOW bridge applies a repeating-key XOR operation and validates a Fletcher-16 checksum. This does not provide cryptographic confidentiality or packet authentication. The current default secret is also predictable, and the CLI allows secret disclosure (addressed separately).

## Proposed design

- Replace XOR and Fletcher-16 as security mechanisms with a vetted AEAD construction supported by the target ESP32 hardware/software stack (for example, AES-GCM or ChaCha20-Poly1305 after resource and compatibility review).
- Provision a high-entropy shared key per bridge network; do not ship a universal default.
- Use a unique nonce for every encrypted frame and define nonce generation/rekey behaviour across reboot.
- Authenticate protocol version, network identifier, packet metadata, and payload as associated data.
- Add replay protection using a bounded sequence/window mechanism.
- Define maximum frame sizes and failure behaviour before parsing/decrypting packet data.
- Provide a documented key rotation and migration strategy.

## Compatibility and operational requirements

- Decide whether old and new bridge frames can coexist during rollout.
- Reject unauthenticated legacy frames after migration.
- Consider broadcast ESP-NOW peer constraints; do not assume unicast peer encryption can be applied unchanged to broadcast.
- Measure RAM, CPU, airtime, and packet-size impact on supported ESP32 boards.

## Acceptance criteria

- Altered, forged, and replayed frames are rejected.
- Nonces are never reused with the same key.
- No default shared secret is present in production builds.
- Valid frames interoperate across supported bridge versions according to the approved migration plan.

## Why this is an RFC

The replacement changes the bridge wire format and key lifecycle. Nonce management, provisioning, broadcast behaviour, and migration must be designed together.