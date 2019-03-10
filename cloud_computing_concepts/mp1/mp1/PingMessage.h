#ifndef _PING_MESSAGE_H_
#define _PING_MESSAGE_H_

#include "BaseMessage.h"

class PingMessage : public BaseMessage {
public:
    PingMessage() : BaseMessage() {}
    PingMessage(MsgTypes msgType,
                Address source,
                Address destination,
                unsigned long protocol_period,
                const vector<MembershipListEntry>& piggybackMembershipList,
                const vector<FailListEntry>& piggybackFailList) : 
                BaseMessage(msgType,
                            source,
                            destination,
                            protocol_period,
                            piggybackMembershipList,
                            piggybackFailList) {}
    string getId() override;
    void decode(string msg) override;
    string encode() override;
    void onReceiveHandler(MP1Node& state) override;
    // TODO: Implement this properly...
    void printMsg() override {};
};

#endif