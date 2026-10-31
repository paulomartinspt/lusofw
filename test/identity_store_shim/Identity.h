#pragma once
#include <Arduino.h>

#ifndef PUB_KEY_SIZE
#define PUB_KEY_SIZE 32
#endif
#ifndef PRV_KEY_SIZE
#define PRV_KEY_SIZE 64
#endif

namespace mesh {
class LocalIdentity {
public:
  bool writeTo(Stream& stream) const {
    const uint8_t pub_key[PUB_KEY_SIZE] = {};
    const uint8_t prv_key[PRV_KEY_SIZE] = {};
    if (stream.write(pub_key, sizeof(pub_key)) != sizeof(pub_key)) return false;
    return stream.write(prv_key, sizeof(prv_key)) == sizeof(prv_key);
  }

  bool readFrom(Stream& stream) {
    uint8_t pub_key[PUB_KEY_SIZE];
    uint8_t prv_key[PRV_KEY_SIZE];
    if (stream.readBytes(pub_key, sizeof(pub_key)) != sizeof(pub_key)) return false;
    return stream.readBytes(prv_key, sizeof(prv_key)) == sizeof(prv_key);
  }
};
}
