#include "PingReqMessage.h"

string PingReqMessage::getId() {
    return source.getAddress() + "|" + route.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingReqMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    MP1Node::copyObj(msgPtr, this->msgType);
    MP1Node::copyObj(msgPtr, this->source);
    MP1Node::copyObj(msgPtr, this->route);
    MP1Node::copyObj(msgPtr, this->destination);
    MP1Node::copyObj(msgPtr, this->protocol_period);
    decodePiggybackLists(msgPtr);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|route|dest_address|protocol_period|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
vector<char> PingReqMessage::encode() {
    vector<char> piggybackMembershipListMsg = MembershipList::encodeTopKMsg(this->piggybackMembershipList);
    vector<char> piggybackFailListMsg = FailList::encodeTopKMsg(this->piggybackFailList);
    
    unsigned msgSize = sizeof(MsgTypes::Types) // MsgType
                        + sizeof(Address) // src_address
                        + sizeof(Address) // route
                        + sizeof(Address) // dest_address
                        + sizeof(unsigned long) // protocol_period
                        + sizeof(unsigned) // pb_ml_size
                        + piggybackMembershipListMsg.size() // pb_ml
                        + sizeof(unsigned) // pb_fl_size
                        + piggybackFailList.size(); // pb_fl
    
    vector<char> msg(msgSize);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::PING_REQ;
    MP1Node::copyMsg(msgStart, type);
    MP1Node::copyMsg(msgStart, this->source);
    MP1Node::copyMsg(msgStart, this->route);
    MP1Node::copyMsg(msgStart, this->destination);
    MP1Node::copyMsg(msgStart, this->protocol_period);

    encodePiggybackLists(msgStart, piggybackMembershipListMsg, piggybackFailListMsg);

#ifdef DEBUGLOG
    cout << "In PingReqMessage::encode()..." << endl;
    cout << "msgSize: " << msgSize << endl;
#endif
    return msg;
}

void PingReqMessage::onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
#ifdef DEBUGLOG
    cout << "In PingReqMessage::onReceiveHandler..." << endl;
#endif    
}

