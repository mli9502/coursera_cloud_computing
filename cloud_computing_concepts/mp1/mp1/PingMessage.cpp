#include "PingMessage.h"

string PingMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    MP1Node::copyObj(msgPtr, this->msgType);
    MP1Node::copyObj(msgPtr, this->source);
    MP1Node::copyObj(msgPtr, this->destination);
    MP1Node::copyObj(msgPtr, this->protocol_period);
    decodePiggybackLists(msgPtr);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|dest_address|protocol_period|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
vector<char> PingMessage::encode() {
    unsigned msgSizeBeforePiggyback = sizeof(MsgTypes::Types) // MsgType
                                        + sizeof(Address) // src_address
                                        + sizeof(Address) // dest_address
                                        + sizeof(unsigned long); // protocol_period
    
    vector<char> msg(msgSizeBeforePiggyback);
    char* msgStart = &msg[0];
    
    MsgTypes::Types type = MsgTypes::PING;
    MP1Node::copyMsg(msgStart, type);
    MP1Node::copyMsg(msgStart, this->source);
    MP1Node::copyMsg(msgStart, this->destination);
    MP1Node::copyMsg(msgStart, this->protocol_period);

    encodeAndAppendPiggybackLists(msg);

#ifdef DEBUGLOG
    cout << "In PingMessage::encode()..." << endl;
    cout << "msgSize: " << msg.size() << endl;
#endif
    return msg;
}

void PingMessage::onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
#ifdef DEBUGLOG
    cout << "In PingMessage::onReceiveHandler..." << endl;
#endif
}