#include <iostream>

#include "Member.h"
#include "AckMessage.h"
#include "gtest/gtest.h"

class MessageTestFixture : public ::testing::Test {
protected:
    MessageTestFixture() : protocol_period(1),
                            incarnation(2),
                            source("1000:10"), 
                            destination("1001:10") {
        vector<Address> membershipListAddrs { Address{"0:0"}, Address{"1:1"} };
        vector<MemberTypes> membershipListTypes { MemberTypes::ALIVE, MemberTypes::SUSPECT };
        vector<int> membershipListIncarnations { 0, 1 };

        for(unsigned i = 0; i < membershipListAddrs.size(); i ++) {
            membershipList.emplace_back(MembershipListEntry(membershipListAddrs[i],
                                                            membershipListTypes[i],
                                                            membershipListIncarnations[i]));
        }

        vector<Address> failListAddrs { Address{"2:2"}, Address{"3:3"} };
        for(unsigned i = 0; i < failListAddrs.size(); i ++) {
            failList.emplace_back(FailListEntry(failListAddrs[i]));
        }
    }
    void SetUp() override {}

    unsigned long protocol_period;
    unsigned long incarnation;
    Address source;
    Address destination;
    vector<MembershipListEntry> membershipList;
    vector<FailListEntry> failList;
};

TEST_F(MessageTestFixture, AckMessage) {
    AckMessage am(MsgTypes::ACK, source, destination, protocol_period, incarnation, membershipList, failList);
    am.printMsg();
    vector<char> encoded = am.encode();
    AckMessage decodedAm;
    decodedAm.decode(encoded);
    decodedAm.printMsg();
}