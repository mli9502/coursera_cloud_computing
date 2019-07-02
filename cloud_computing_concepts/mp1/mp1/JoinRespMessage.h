#ifndef _JOIN_RESP_MESSAGE_H_
#define _JOIN_RESP_MESSAGE_H_

#include "BaseMessage.h"

class JoinRespMessage : public BaseMessage {
public:
    JoinRespMessage() : BaseMessage() {}
    JoinRespMessage(MsgTypes::Types msgType,
                Address source,
                Address destination,
                const vector<MembershipListEntry>& membershipList,
                const vector<FailListEntry>& failList) : BaseMessage() {
        this->source = source;
        this->destination = destination;
        // piggybackMembershipList and piggybackFailList are actually the membership list at coordinator, 
        // at the time this message is sent. The max number of entries that is sent in may be applied.
        this->piggybackMembershipList = membershipList;
        this->piggybackFailList = failList;
    }
    string getId() override;
    void decode(const vector<char>& msg) override;
    vector<char> encode() override;
    shared_ptr<BaseMessage> onReceiveHandler(MP1Node& node) override;
    void printMsg() override;
    // The maximum number of entries (both membershipList and failList) that we should include in this message.
    static const unsigned MAX_LIST_ENTRY;
};

#endif