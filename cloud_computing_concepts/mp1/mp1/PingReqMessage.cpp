#include "PingReqMessage.h"

string PingReqMessage::getId() {
    return source.getAddress() + "|" + route.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingReqMessage::decode(string msg) {
    char* msgPtr = &(msg[0]);
    MP1Node::copyObj(msgPtr, this->msgType);
    MP1Node::copyObj(msgPtr, this->source);
    MP1Node::copyObj(msgPtr, this->route);
    MP1Node::copyObj(msgPtr, this->destination);
    MP1Node::copyObj(msgPtr, this->protocol_period);
    MembershipList::decodeTopKMsg(msgPtr, this->piggybackMembershipList);
    FailList::decodeTopKMsg(msgPtr, this->piggybackFailList);    
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|route|dest_address|protocol_period|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
string PingReqMessage::encode() {
    string piggybackMembershipListMsg = MembershipList::encodeTopKMsg(this->piggybackMembershipList);
    string piggybackFailListMsg = FailList::encodeTopKMsg(this->piggybackFailList);
    
    unsigned msgSize = sizeof(MsgTypes) // MsgType
                        + sizeof(Address) // src_address
                        + sizeof(Address) // route
                        + sizeof(Address) // dest_address
                        + sizeof(unsigned long) // protocol_period
                        + sizeof(unsigned) // pb_ml_size
                        + piggybackMembershipListMsg.size() // pb_ml
                        + sizeof(unsigned) // pb_fl_size
                        + piggybackFailList.size(); // pb_fl
    
    char* msg = new char[msgSize];
    auto msgStart = msg;
    MsgTypes type = MsgTypes::PING;
    MP1Node::copyMsg(msgStart, type);
    MP1Node::copyMsg(msgStart, this->source);
    MP1Node::copyMsg(msgStart, this->route);
    MP1Node::copyMsg(msgStart, this->destination);
    MP1Node::copyMsg(msgStart, this->protocol_period);
    unsigned pbMlSize = this->piggybackMembershipList.size();
    MP1Node::copyMsg(msgStart, pbMlSize);
    memcpy(msgStart, piggybackMembershipListMsg.c_str(), piggybackMembershipListMsg.size());
    msgStart += piggybackMembershipListMsg.size();
    unsigned pbFlSize = this->piggybackFailList.size();
    MP1Node::copyMsg(msgStart, pbFlSize);
    memcpy(msgStart, piggybackFailListMsg.c_str(), piggybackFailListMsg.size());
    msgStart += piggybackFailListMsg.size();

    string rtn(msg, msgSize);
#ifdef DEBUGLOG
    cout << "In PingReqMessage::encode()..." << endl;
    cout << "msgSize: " << msgSize << endl;
    cout << "msg: " << rtn << endl;
#endif
    return rtn;
}

void PingReqMessage::onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
#ifdef DEBUGLOG
    cout << "In PingReqMessage::onReceiveHandler..." << endl;
#endif    
}

