#include "MessageDecoder.h"

MsgTypes::Types MessageDecoder::getTypeFromMsg(const vector<char>& msg) {
    MessageHdr* tmp = (MessageHdr*)malloc(sizeof(MessageHdr));
    memcpy(tmp, &msg[0], sizeof(MessageHdr));
    MsgTypes::Types rtn = tmp->msgType;
    free(tmp);
    return rtn;
}

MsgTypes::Types MessageDecoder::getTypeFromMsg(char* msg, unsigned size) {
    vector<char> msgVec(msg, msg + size);
    return MessageDecoder::getTypeFromMsg(msgVec);
}

// Every time a node receives a message, 
// we should call this method to decode the message.
// Then, use the return pointer, we can call the onReceiveHandler method to handle the received message.
shared_ptr<BaseMessage> MessageDecoder::decode(const vector<char>& msg) {
    MsgTypes::Types type = MessageDecoder::getTypeFromMsg(msg);
    shared_ptr<BaseMessage> rtn;
    switch(type) {
        case MsgTypes::Types::PING: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING!" << endl;
#endif
            rtn.reset(new PingMessage());
            break;
        }
        case MsgTypes::Types::PING_REQ: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING_REQ!" << endl;
#endif  
            rtn.reset(new PingReqMessage());
            break;
        }
        case MsgTypes::Types::ACK: {
#ifdef DEBUGLOG
            cout << "Receiving message of type ACK!" << endl;
#endif
            rtn.reset(new AckMessage());
            break;            
        }
        case MsgTypes::Types::JOINREQ: {
#ifdef DEBUGLOG
            cout << "Receiving message of type JOINREQ!" << endl;
#endif
            rtn.reset(new JoinReqMessage());
        }
        default: {
#ifdef DEBUGLOG
            cout << "Receiving message of types that are currently not supported!" << endl;
            cout << "type: " << type << endl;
#endif
            break;
        }
    }
    if(!rtn) {
        return rtn;
    }
    // Actually decode the message.
    rtn->decode(msg);
    return rtn;
}

shared_ptr<BaseMessage> MessageDecoder::decode(char* msg, unsigned size) {
    vector<char> msgVec(msg, msg + size);
    return MessageDecoder::decode(msgVec);
}
