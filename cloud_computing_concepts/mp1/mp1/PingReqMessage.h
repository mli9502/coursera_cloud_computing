#ifndef _PING_REQ_MESSAGE_H_
#define _PING_REQ_MESSAGE_H_

#include "BaseMessage.h"

class PingReqMessage : public BaseMessage {
public:
    PingReqMessage() : BaseMessage() {}
    PingReqMessage(MsgTypes::Types msgType,
                    Address source,
                    Address route,
                    Address destination,
                    unsigned long protocol_period,
                    const vector<MembershipListEntry>& piggybackMembershipList,
                    const vector<FailListEntry>& piggybackFailList) : 
                    BaseMessage(msgType,
                                source,
                                destination,
                                protocol_period,
                                piggybackMembershipList,
                                piggybackFailList),
                    route(route) {}

    Address getRoute() {
        return route;
    }
    string getId() override;
    void decode(const vector<char>& msg) override;
    vector<char> encode() override;
    shared_ptr<BaseMessage> onReceiveHandler(MP1Node& node) override;
    void printMsg() override;

private:
    Address route;
};

#endif