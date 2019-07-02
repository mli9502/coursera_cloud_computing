#ifndef _JOIN_REQ_MESSAGE_H_
#define _JOIN_REQ_MESSAGE_H_

#include "BaseMessage.h"

class JoinReqMessage : public BaseMessage {
public:
    JoinReqMessage() : BaseMessage() {}
    JoinReqMessage(MsgTypes::Types msgType,
                Address source,
                Address destination) : BaseMessage() {
        this->source = source;
        this->destination = destination;
    }
    string getId() override;
    void decode(const vector<char>& msg) override;
    vector<char> encode() override;
    shared_ptr<BaseMessage> onReceiveHandler(MP1Node& node) override;
    void printMsg() override;
};

#endif