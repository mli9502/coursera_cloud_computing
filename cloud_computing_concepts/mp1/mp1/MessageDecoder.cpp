#include "MessageDecoder.h"

MsgTypes MessageDecoder::getTypeFromMsg(const string& msg) {
    MessageHdr* tmp = (MessageHdr*)malloc(sizeof(MessageHdr));
    memcpy(tmp, msg.c_str(), sizeof(MessageHdr));
    MsgTypes rtn = tmp->msgType;
    free(tmp);
    return rtn;
}

// Every time a node receives a message, 
// we should call this method to decode the message.
// Then, use the return pointer, we can call the onReceiveHandler method to handle the received message.
shared_ptr<BaseMessage> decode(const string& msg) {
    MsgTypes type = MessageDecoder::getTypeFromMsg(msg);
    shared_ptr<BaseMessage> rtn;
    switch(type) {
        case PING: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING!" << endl;
            cout << "msg: " << msg << endl;
#endif
            rtn.reset(new PingMessage());
            break;
        }
        case PING_REQ: {
#ifdef DEBUGLOG
            cout << "Receiving message of type PING_REQ!" << endl;
            cout << "msg: " << msg << endl;
#endif  
            rtn.reset(new PingReqMessage());
            break;
        }
        case ACK: {
#ifdef DEBUGLOG
            cout << "Receiving message of type ACK!" << endl;
            cout << "msg: " << msg << endl;
#endif
            rtn.reset(new AckMessage());
            break;            
        }
        default: {
#ifdef DEBUGLOG
            cout << "Receiving message of types that are currently not supported!" << endl;
            cout << "type: " << type << endl;
            cout << "msg: " << msg << endl;
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