#include <iostream>

#include "Member.h"
#include "AckMessage.h"
#include "gtest/gtest.h"

TEST(MessageTest, AckMessage) {
    Address source("1000:10");
    Address destination("1001:10");
    unsigned long protocol_period = 0;
    unsigned long incarnation = 0;
    vector<Address> membershipListAddrs { Address{"0:0"}, Address{"1:1"} };
    vector<MemberTypes> membershipListTypes { MemberTypes::ALIVE, MemberTypes::SUSPECT };
    vector<int> membershipListIncarnations { 0, 1 };
    vector<MembershipListEntry> mles;
    for(int i = 0; i < membershipListAddrs.size(); i ++) {
        mles.emplace_back(MembershipListEntry(membershipListAddrs[i],
                                                membershipListTypes[i],
                                                membershipListIncarnations[i]));
    }
    vector<Address> failListAddrs { Address{"2:2"}, Address{"3:3"} };
    vector<FailListEntry> fle;
    for(int i = 0; i < failListAddrs.size(); i ++) {
        fle.emplace_back(FailListEntry(failListAddrs[i]));
    }
    EXPECT_EQ(1, 1);
}