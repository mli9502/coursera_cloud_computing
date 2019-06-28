#include <iostream>

#include "Member.h"
#include "AckMessage.h"
#include "PingMessage.h"
#include "PingReqMessage.h"
#include "gtest/gtest.h"

namespace 
{

template <typename T>
bool isListEq(const T& v1, const T& v2) {
    if(v1.size() != v2.size()) {
        return false;
    }
    auto it1 = v1.begin();
    auto it2 = v2.begin();
    while(it1 != v1.end() && it2 != v2.end()) {
        if(*it1 != *it2) {
            return false;
        }
        it1 ++;
        it2 ++;
    }
    return true;
}

}

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
    // Encode the decoded message.
    vector<char> reEncoded = decodedAm.encode();
    EXPECT_TRUE(isListEq(encoded, reEncoded));
}

TEST_F(MessageTestFixture, PingMessage) {
    PingMessage pm(MsgTypes::PING, source, destination, protocol_period, membershipList, failList);
    pm.printMsg();
    vector<char> encoded = pm.encode();
    PingMessage decodedPm;
    decodedPm.decode(encoded);
    decodedPm.printMsg();
    vector<char> reEncoded = decodedPm.encode();
    EXPECT_TRUE(isListEq(encoded, reEncoded));
}

TEST_F(MessageTestFixture, PingReqMessage) {
    Address route { "1002:10" };
    PingReqMessage prm(MsgTypes::PING_REQ, source, route, destination, protocol_period, membershipList, failList);
    prm.printMsg();
    vector<char> encoded = prm.encode();
    PingReqMessage decodedPrm;
    decodedPrm.decode(encoded);
    decodedPrm.printMsg();
    vector<char> reEncoded = decodedPrm.encode();
    EXPECT_TRUE(isListEq(encoded, reEncoded));
}