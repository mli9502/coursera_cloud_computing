#include <TimeoutMap.h>
#include <Member.h>

#include "gtest/gtest.h"

namespace {
const int TTL = 5;
}

class TimeoutMapTestFixture : public ::testing::Test {
protected:
    TimeoutMapTestFixture() : _tm(TTL) {}

    void SetUp() override {
        _tm.insert("id0", Address{"0:0"});
        _tm.insert("id1", Address{"1:1"});
    }

    TimeoutMap<string, Address> _tm;
};

TEST_F(TimeoutMapTestFixture, TestContains) {
    ASSERT_TRUE(_tm.contains("id0"));
    ASSERT_FALSE(_tm.contains("random_id"));
}

TEST_F(TimeoutMapTestFixture, TestInsert) {
    _tm.insert("id2", Address{"2:2"});
    ASSERT_TRUE(_tm.contains("id2"));
}

TEST_F(TimeoutMapTestFixture, TestGet) {
    Address* val = _tm.get("id0");
    ASSERT_EQ(val->getAddress(), "0:0");

    val = _tm.get("random_id");
    ASSERT_EQ(val, nullptr);
}

TEST_F(TimeoutMapTestFixture, TestUpdate) {
    _tm.update("id0", Address{"5:5"}, false);
    ASSERT_EQ(_tm.get("id0")->getAddress(), "5:5");
}

// This is how tick works.
// If the ttl is set to 5, then, at the 5th tick, the entry will be evicted.
TEST_F(TimeoutMapTestFixture, TestTick) {
    for(int i = 0; i < 5; i ++) {
        if(i == 3) {
            _tm.insert("id2", Address{"2:2"});
        }
        _tm.tick();
    }
    ASSERT_FALSE(_tm.contains("id0"));
    ASSERT_FALSE(_tm.contains("id1"));
    ASSERT_TRUE(_tm.contains("id2"));
    _tm.tick();
    ASSERT_TRUE(_tm.contains("id2"));
    _tm.tick();
    ASSERT_TRUE(_tm.contains("id2"));
    _tm.tick();
    ASSERT_FALSE(_tm.contains("id2"));
}