#ifndef _MESSAGE_DECODER_H_
#define _MESSAGE_DECODER_H_

#include "PingMessage.h"
#include "PingReqMessage.h"
#include "AckMessage.h"

/**
 * class used to decode the messages.
 * this class will decode the message based on the initial message type.
 * and return the approate object.
 */ 
class MessageDecoder {
public:
    static MsgTypes getTypeFromMsg(const string& msg);
    static shared_ptr<BaseMessage> decode(const string& msg);
};

#endif