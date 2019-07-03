#ifndef _MESSAGE_DECODER_H_
#define _MESSAGE_DECODER_H_

#include "PingMessage.h"
#include "PingReqMessage.h"
#include "AckMessage.h"
#include "JoinReqMessage.h"
#include "JoinRespMessage.h"

/**
 * class used to decode the messages.
 * this class will decode the message based on the initial message type.
 * and return the approate object.
 */ 
class MessageDecoder {
public:
    static MsgTypes::Types getTypeFromMsg(const vector<char>& msg);
    static MsgTypes::Types getTypeFromMsg(char* msg, unsigned size);
    static shared_ptr<BaseMessage> decode(const vector<char>& msg);
    static shared_ptr<BaseMessage> decode(char* msg, unsigned size);
};

#endif