// SPDX-FileCopyrightText: 2026 Haluan Irsad
// SPDX-License-Identifier: AGPL-3.0-only OR LicenseRef-Commercial

#include "cache/l1_cache.h"

#include <cstring>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace bytetaper::cache;

class L1CacheConcurrencyTest : public ::testing::Test {
protected:
    std::unique_ptr<L1Cache> cache_;

    void SetUp() override {
        cache_ = std::make_unique<L1Cache>();
        l1_init(cache_.get());
    }
};

TEST_F(L1CacheConcurrencyTest, ConcurrentPutsOnSameShard) {
    constexpr int kNumThreads = 64;
    std::vector<std::thread> threads;

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([this, i]() {
            CacheEntry entry{};
            std::string key = "same_key";
            std::strncpy(entry.key, key.c_str(), kCacheKeyMaxLen);
            entry.status_code = 200;

            std::string body = "body_" + std::to_string(i);
            entry.body = body.data();
            entry.body_len = body.size();

            l1_put(cache_.get(), entry);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    CacheEntry out{};
    bool hit = l1_get(cache_.get(), "same_key", 0, &out);
    EXPECT_TRUE(hit);
    EXPECT_EQ(out.status_code, 200);
}

TEST_F(L1CacheConcurrencyTest, ConcurrentPutsAndGets) {
    constexpr int kNumThreads = 32;
    std::vector<std::thread> writers;
    std::vector<std::thread> readers;

    for (int i = 0; i < kNumThreads; ++i) {
        writers.emplace_back([this, i]() {
            CacheEntry entry{};
            std::string key = "key_" + std::to_string(i);
            std::strncpy(entry.key, key.c_str(), kCacheKeyMaxLen);
            entry.status_code = 200;

            std::string body = "body_data_" + std::to_string(i);
            entry.body = body.data();
            entry.body_len = body.size();

            l1_put(cache_.get(), entry);
        });

        readers.emplace_back([this, i]() {
            CacheEntry out{};
            std::string key = "key_" + std::to_string(i);
            // It might miss if the writer hasn't run yet, which is fine.
            // We just want to ensure no crashes or data races.
            l1_get(cache_.get(), key.c_str(), 0, &out);
        });
    }

    for (auto& t : writers) {
        t.join();
    }
    for (auto& t : readers) {
        t.join();
    }

    // Now verify everything is there
    for (int i = 0; i < kNumThreads; ++i) {
        CacheEntry out{};
        std::string key = "key_" + std::to_string(i);
        bool hit = l1_get(cache_.get(), key.c_str(), 0, &out);
        EXPECT_TRUE(hit) << "Missing key: " << key;
        if (hit) {
            std::string expected_body = "body_data_" + std::to_string(i);
            std::string actual_body(out.body, out.body_len);
            EXPECT_EQ(expected_body, actual_body);
        }
    }
}
