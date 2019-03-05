#ifndef _BASE_MESSAGE_H_
#define _BASE_MESSAGE_H_

#include "stdincludes.h"

#include "MP1Node.h"

class BaseMessage {
public:
    BaseMessage() {};
    // ctor in order to encode.
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
    virtual void decode(string msg) = 0;
    virtual string encode() = 0;
    // This method implements what we should do when we receive a type of message.
    virtual void onReceiveHandler(MP1Node& state) = 0;
    
    virtual void printMsg() = 0;

protected:
    MsgTypes msgType;
    Address source;
    Address destination;
    unsigned long protocol_period;
    vector<MembershipListEntry> piggybackMembershipList;
    vector<FailListEntry> piggybackFailList;
};

#endif