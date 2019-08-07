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
        _tm.insert("id2", Address{"2:2"});
        _tm.insert("id3", Address{"3:3"});
        _tm.insert("id4", Address{"4:4"});
    }

    TimeoutMap<string, Address> _tm;
};

TEST_F(TimeoutMapTestFixture, TestContains) {
    ASSERT_TRUE(_tm.contains("id0"));
    ASSERT_FALSE(_tm.contains("random_id"));
}

TEST_F(TimeoutMapTestFixture, TestInsert) {
    _tm.insert("id5", Address{"5:5"});
    ASSERT_TRUE(_tm.contains("id5"));
}

TEST_F(TimeoutMapTestFixture, TestGet) {
    Address* val = _tm.get("id0");
    ASSERT_EQ(val->getAddress(), "0:0");

    val = _tm.get("random_id");
    ASSERT_EQ(val, nullptr);
}