#pragma once
// Small, isolated Arduino/FS shim for IdentityStore host tests.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#define MESH_DEBUG_PRINTLN(...) do {} while (0)

class Stream {
public:
  virtual ~Stream() = default;
  virtual size_t write(const uint8_t* data, size_t length) = 0;
  virtual size_t readBytes(uint8_t* data, size_t length) = 0;
};

class File : public Stream {
public:
  File() = default;

  static File makeWrite(std::vector<uint8_t>* storage, int short_write_call, size_t short_write_bytes) {
    File file;
    file._storage = storage;
    file._valid = storage != nullptr;
    file._write_mode = true;
    file._short_write_call = short_write_call;
    file._short_write_bytes = short_write_bytes;
    return file;
  }

  static File makeRead(std::vector<uint8_t>* storage) {
    File file;
    file._storage = storage;
    file._valid = storage != nullptr;
    file._write_mode = false;
    return file;
  }

  explicit operator bool() const { return _valid && _storage != nullptr; }

  size_t write(const uint8_t* data, size_t length) override {
    if (!_valid || !_write_mode || !_storage) return 0;
    ++_write_calls;
    size_t written = length;
    if (_short_write_call > 0 && _write_calls == _short_write_call) {
      written = std::min(length, _short_write_bytes);
    }
    _storage->insert(_storage->end(), data, data + written);
    return written;
  }

  size_t write(uint8_t value) { return write(&value, 1); }

  size_t readBytes(uint8_t* data, size_t length) override {
    if (!_valid || _write_mode || !_storage) return 0;
    const size_t available = _position < _storage->size() ? _storage->size() - _position : 0;
    const size_t count = std::min(available, length);
    if (count) std::memcpy(data, _storage->data() + _position, count);
    _position += count;
    return count;
  }

  size_t read(uint8_t* data, size_t length) { return readBytes(data, length); }

  int read() {
    uint8_t value = 0;
    if (readBytes(&value, 1) != 1) return -1;
    return value;
  }

  void close() { _valid = false; }

private:
  std::vector<uint8_t>* _storage = nullptr;
  size_t _position = 0;
  int _write_calls = 0;
  int _short_write_call = 0;
  size_t _short_write_bytes = 0;
  bool _write_mode = false;
  bool _valid = false;
};

class FILESYSTEM {
public:
  File open(const char* path, const char* mode) {
    if (!path || !mode) return File();
    if (mode[0] == 'w') {
      if (fail_write_open) return File();
      auto it = _files.find(path);
      if (it == _files.end()) it = _files.emplace(path, std::vector<uint8_t>()).first;
      it->second.clear();
      return File::makeWrite(&it->second, short_write_call, short_write_bytes);
    }
    auto it = _files.find(path);
    if (it == _files.end()) return File();
    return File::makeRead(&it->second);
  }

  bool exists(const char* path) const { return _files.find(path) != _files.end(); }
  bool remove(const char* path) { return _files.erase(path) != 0; }

  const std::vector<uint8_t>* bytes(const char* path) const {
    auto it = _files.find(path);
    return it == _files.end() ? nullptr : &it->second;
  }

  bool fail_write_open = false;
  int short_write_call = 0;
  size_t short_write_bytes = 0;

private:
  std::map<std::string, std::vector<uint8_t>> _files;
};
