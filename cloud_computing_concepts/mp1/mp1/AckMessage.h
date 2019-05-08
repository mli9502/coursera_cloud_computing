#ifndef _ACK_MESSAGE_H_
#define _ACK_MESSAGE_H_

#include "BaseMessage.h"

class AckMessage : public BaseMessage {
public:
    AckMessage() : BaseMessage() {}
    AckMessage(MsgTypes::Types msgType,
                Address source,
                Address destination,
                unsigned long protocol_period,
                unsigned long incarnation,
                const vector<MembershipListEntry>& piggybackMembershipList,
                const vector<FailListEntry>& piggybackFailList) : 
                BaseMessage(msgType,
                            source,
                            destination,
                            protocol_period,
                            piggybackMembershipList,
                            piggybackFailList),
                incarnation(incarnation) {}
    
    unsigned long getIncarnation() {
        return incarnation;
    }
    string getId() override;
    void decode(string msg) override;
    string encode() override;
    void onReceiveHandler(MP1Node& state) override;

    void printMsg() override;
    
private:
    unsigned long incarnation;
};

#endif