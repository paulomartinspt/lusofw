#include <gtest/gtest.h>

#include <Identity.h>
#include <helpers/IdentityStore.h>

namespace {
const char* kIdentityPath = "/identity/main.id";
}

TEST(IdentityStore, IdentityOnlySaveSucceedsAndPersistsFixedKeyRecord) {
  fs::FS fs;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_TRUE(store.save("main", identity));
  const auto* bytes = fs.bytes(kIdentityPath);
  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->size(), 32u + 64u);
}

TEST(IdentityStore, IdentityOnlySaveReturnsFalseOnShortPrivateKeyWrite) {
  fs::FS fs;
  fs.short_write_call = 2;
  fs.short_write_bytes = 7;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_FALSE(store.save("main", identity));
  const auto* bytes = fs.bytes(kIdentityPath);
  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->size(), 32u + 7u);
}

TEST(IdentityStore, SaveWithDisplayNameSucceedsAndPersistsFixedRecord) {
  fs::FS fs;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_TRUE(store.save("main", identity, "Repeater"));
  const auto* bytes = fs.bytes(kIdentityPath);
  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->size(), 32u + 64u + 32u);
}

TEST(IdentityStore, SaveWithDisplayNameReturnsFalseOnShortNameWrite) {
  fs::FS fs;
  fs.short_write_call = 3;  // public key, private key, then fixed-size name
  fs.short_write_bytes = 9;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_FALSE(store.save("main", identity, "Repeater"));
  const auto* bytes = fs.bytes(kIdentityPath);
  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->size(), 32u + 64u + 9u);
}

TEST(IdentityStore, SaveWithDisplayNameReturnsFalseOnIdentitySerializationFailure) {
  fs::FS fs;
  fs.short_write_call = 2;
  fs.short_write_bytes = 5;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_FALSE(store.save("main", identity, "Repeater"));
}

TEST(IdentityStore, SaveReturnsFalseWhenFileCannotBeOpened) {
  fs::FS fs;
  fs.fail_write_open = true;
  IdentityStore store(fs, "/identity");
  mesh::LocalIdentity identity;

  EXPECT_FALSE(store.save("main", identity));
  EXPECT_FALSE(store.save("main", identity, "Repeater"));
}
