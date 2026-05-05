// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/cache_entry.h"
#include "cache/l1_cache.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

namespace bytetaper::cache {

class L1CacheConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        l1_init(&l1);
    }

    L1Cache l1;
};

TEST_F(L1CacheConcurrencyTest, L1ConcurrentPutFromMultipleThreads) {
    constexpr int kThreadCount = 16;
    constexpr int kEntriesPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> start{ false };

    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([this, i, &start] {
            while (!start)
                std::this_thread::yield();
            for (int j = 0; j < kEntriesPerThread; ++j) {
                CacheEntry e{};
                std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
                ::strncpy(e.key, key.c_str(), sizeof(e.key) - 1);
                e.expires_at_epoch_ms = 9999999999;
                l1_put(&l1, e);
            }
        });
    }

    start = true;
    for (auto& t : threads) {
        t.join();
    }

    // Verify all entries are present
    for (int i = 0; i < kThreadCount; ++i) {
        for (int j = 0; j < kEntriesPerThread; ++j) {
            CacheEntry hit{};
            char body_buf[kL1MaxBodySize];
            std::string key = "key_" + std::to_string(i) + "_" + std::to_string(j);
            bool found = l1_get(&l1, key.c_str(), 0, &hit, body_buf, sizeof(body_buf));
            EXPECT_TRUE(found) << "Key not found: " << key;
            EXPECT_STREQ(hit.key, key.c_str());
        }
    }
}

TEST_F(L1CacheConcurrencyTest, L1ConcurrentGetPutSameKey) {
    constexpr int kWriterCount = 8;
    constexpr int kReaderCount = 8;
    constexpr int kIterations = 1000;
    const char* kSharedKey = "shared_key";

    std::vector<std::thread> threads;
    std::atomic<bool> start{ false };

    for (int i = 0; i < kWriterCount; ++i) {
        threads.emplace_back([this, &start, kSharedKey] {
            while (!start)
                std::this_thread::yield();
            for (int j = 0; j < kIterations; ++j) {
                CacheEntry e{};
                ::strncpy(e.key, kSharedKey, sizeof(e.key) - 1);
                e.expires_at_epoch_ms = 9999999999;

                char local_body[1024];
                std::memset(local_body, (char) (j % 256), sizeof(local_body));
                e.body = local_body;
                e.body_len = sizeof(local_body);
                l1_put(&l1, e);
            }
        });
    }

    for (int i = 0; i < kReaderCount; ++i) {
        threads.emplace_back([this, &start, kSharedKey] {
            while (!start)
                std::this_thread::yield();
            for (int j = 0; j < kIterations; ++j) {
                CacheEntry hit{};
                char body_buf[kL1MaxBodySize];
                bool found = l1_get(&l1, kSharedKey, 0, &hit, body_buf, sizeof(body_buf));
                if (found) {
                    // Check if the body is consistent (all bytes should be the same)
                    char expected = hit.body[0];
                    for (std::size_t k = 1; k < hit.body_len; ++k) {
                        ASSERT_EQ(hit.body[k], expected) << "Torn read detected!";
                    }
                }
            }
        });
    }

    start = true;
    for (auto& t : threads) {
        t.join();
    }
}

TEST_F(L1CacheConcurrencyTest, L1ConcurrentPutAndExpiry) {
    constexpr int kThreadCount = 8;
    constexpr int kIterations = 500;
    std::vector<std::thread> threads;
    std::atomic<bool> start{ false };
    std::atomic<int64_t> global_now_ms{ 1000 };

    for (int i = 0; i < kThreadCount; ++i) {
        threads.emplace_back([this, i, &start, &global_now_ms] {
            while (!start)
                std::this_thread::yield();
            for (int j = 0; j < kIterations; ++j) {
                CacheEntry e{};
                std::string key = "expiry_key_" + std::to_string(i) + "_" + std::to_string(j);
                ::strncpy(e.key, key.c_str(), sizeof(e.key) - 1);
                e.expires_at_epoch_ms = global_now_ms.load() + 1000;
                l1_put(&l1, e);

                CacheEntry hit{};
                char body_buf[kL1MaxBodySize];
                // Immediate get should succeed
                EXPECT_TRUE(l1_get(&l1, key.c_str(), global_now_ms.load(), &hit, body_buf,
                                   sizeof(body_buf)));

                // Get with future time should fail
                EXPECT_FALSE(l1_get(&l1, key.c_str(), global_now_ms.load() + 2000, &hit, body_buf,
                                    sizeof(body_buf)));
            }
        });
    }

    start = true;
    for (auto& t : threads) {
        t.join();
    }
}

} // namespace bytetaper::cache
