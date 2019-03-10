#ifndef _PING_REQ_MESSAGE_H_
#define _PING_REQ_MESSAGE_H_

#include "BaseMessage.h"

class PingReqMessage : public BaseMessage {
public:
    PingReqMessage() : BaseMessage() {}
    PingReqMessage(MsgTypes msgType,
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
    void decode(string msg) override;
    string encode() override;
    void onReceiveHandler(MP1Node& state) override;
    // TODO: Need to implement this properly.
    void printMsg() override {}

private:
    Address route;
};

#endif