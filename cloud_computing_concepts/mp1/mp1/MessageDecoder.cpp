#include "MessageDecoder.h"

MsgTypes::Types MessageDecoder::getTypeFromMsg(const vector<char>& msg) {
    MessageHdr* tmp = (MessageHdr*)malloc(sizeof(MessageHdr));
    memcpy(tmp, &msg[0], sizeof(MessageHdr));
    MsgTypes::Types rtn = tmp->msgType;
    free(tmp);
    return rtn;
}

// Every time a node receives a message, 
// we should call this method to decode the message.
// Then, use the return pointer, we can call the onReceiveHandler method to handle the received message.
shared_ptr<BaseMessage> decode(const vector<char>& msg) {
    MsgTypes::Types type = MessageDecoder::getTypeFromMsg(msg);
    shared_ptr<BaseMessage> rtn;
    switch(type) {
        case MsgTypes::PING: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING!" << endl;
#endif
            rtn.reset(new PingMessage());
            break;
        }
        case MsgTypes::PING_REQ: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING_REQ!" << endl;
#endif  
            rtn.reset(new PingReqMessage());
            break;
        }
        case MsgTypes::ACK: {
#ifdef DEBUGLOG
            cout << "Receiving message of type ACK!" << endl;
#endif
            rtn.reset(new AckMessage());
            break;            
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