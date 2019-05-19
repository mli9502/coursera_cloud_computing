#include "PingReqMessage.h"

string PingReqMessage::getId() {
    return source.getAddress() + "|" + route.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingReqMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    copyObj(msgPtr, this->msgType);
    copyObj(msgPtr, this->source);
    copyObj(msgPtr, this->route);
    copyObj(msgPtr, this->destination);
    copyObj(msgPtr, this->protocol_period);
    decodePiggybackLists(msgPtr);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|route|dest_address|protocol_period|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
vector<char> PingReqMessage::encode() {
    unsigned msgSizeBeforePiggyback = sizeof(MsgTypes::Types) // MsgType
                                        + sizeof(Address) // src_address
                                        + sizeof(Address) // route
                                        + sizeof(Address) // dest_address
                                        + sizeof(unsigned long); // protocol_period

    vector<char> msg(msgSizeBeforePiggyback);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::PING_REQ;
    copyMsg(msgStart, type);
    copyMsg(msgStart, this->source);
    copyMsg(msgStart, this->route);
    copyMsg(msgStart, this->destination);
    copyMsg(msgStart, this->protocol_period);

    encodeAndAppendPiggybackLists(msg);

#ifdef DEBUGLOG
    cout << "In PingReqMessage::encode()..." << endl;
    cout << "msgSize: " << msg.size() << endl;
#endif
    return msg;
}

void PingReqMessage::onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
#ifdef DEBUGLOG
    cout << "In PingReqMessage::onReceiveHandler..." << endl;
#endif    
}

