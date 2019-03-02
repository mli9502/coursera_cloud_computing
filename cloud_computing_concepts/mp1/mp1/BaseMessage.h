#ifndef _BASE_MESSAGE_H_
#define _BASE_MESSAGE_H_

#include "include_std.h"

#include "MP1Node.h"

class BaseMessage {
public:
    BaseMessage() {};
    // ctor for encode.
    // TODO: Need to implement this in child classes.
    BaseMessage(MsgTypes msgType,
                Address source,
                Address destination,
                unsigned long protocol_period,
                const vector<MembershipListEntry>& piggybackMembershipList,
                const vector<FailListEntry>& piggybackFailList) :
                msgType(msgType),
                source(source),
                destination(destination),
                protocol_period(protocol_period),
                piggybackMembershipList(piggybackMembershipList),
                piggybackFailList(piggybackFailList) {}

    ~BaseMessage() {};

    MsgTypes getMsgType() {
        return msgType;
    }
    Address getSrc() {
        return source;
    }
    Address getDest() {
        return destination;
    }
    const vector<MembershipListEntry>& getMembershipListInPiggyback() {
        return piggybackMembershipList;
    }
    const vector<FailListEntry>& getFailListInPiggyback() {
        return piggybackFailList;
    } 
    // PING: source|destination|protocol_period.
    // PING_REQ: source|route|destination|protocol_period.
    // ACK: destination|source|protocol_period.
    virtual string getId() = 0;
    // TODO: Implement decode and encode.
    virtual shared_ptr<BaseMessage> decode(const string& msg) = 0;
    virtual string encode() = 0;

protected:
    MsgTypes msgType;
    Address source;
    Address destination;
    unsigned long protocol_period;
    vector<MembershipListEntry> piggybackMembershipList;
    vector<FailListEntry> piggybackFailList;
};

#endif