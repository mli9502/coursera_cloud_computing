#include "AckMessage.h"

#include <bitset>

string AckMessage::getId() {
    return destination.getAddress() + "|" + source.getAddress() + "|" + to_string(protocol_period);
}

void AckMessage::decode(string msg) {
    std::cout << "----------------------------" << std::endl;
    for(unsigned i = 0; i < msg.size(); i ++) {
        std::cout << std::bitset<8>(unsigned(msg[i])) << std::endl; 
    }
    char* msgPtr = &(msg[0]);
    std::cout << sizeof(MsgTypes::Types) << std::endl;
    std::cout << "msgPtr before: " << std::hex << static_cast<void*>(msgPtr) << std::endl;
    MP1Node::copyObj(msgPtr, this->msgType);
    std::cout << "msgPtr after: " << std::hex << static_cast<void*>(msgPtr) << std::endl;
    std::cout << sizeof(Address) << std::endl;
    std::cout << "msgType: " << MsgTypes::to_string(this->msgType) << std::endl;
    std::cout << "msgPtr before: " << std::hex << static_cast<void*>(msgPtr) << std::endl;
    MP1Node::copyObj(msgPtr, this->source);
    std::cout << "msgPtr after: " << std::hex << static_cast<void*>(msgPtr) << std::endl;
    std::cout << "source: " << this->source.getAddress() << std::endl;
    MP1Node::copyObj(msgPtr, this->destination);
    std::cout << "destination: " << this->destination.getAddress() << std::endl;
    MP1Node::copyObj(msgPtr, this->protocol_period);
    std::cout << "protocol_period: " << this->protocol_period << std::endl;
    MP1Node::copyObj(msgPtr, this->incarnation);
    std::cout << "incarnation: " << this->incarnation << std::endl;
    return;
    MembershipList::decodeTopKMsg(msgPtr, this->piggybackMembershipList);
    FailList::decodeTopKMsg(msgPtr, this->piggybackFailList);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|dest_address|protocol_period|incarnation_#|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
string AckMessage::encode() {
    vector<char> piggybackMembershipListMsg = MembershipList::encodeTopKMsg(this->piggybackMembershipList);
    vector<char> piggybackFailListMsg = FailList::encodeTopKMsg(this->piggybackFailList);
    
    unsigned msgSize = sizeof(MsgTypes::Types) // MsgType
                        + sizeof(Address) // src_address
                        + sizeof(Address) // dest_address
                        + sizeof(unsigned long) // protocol_period
                        + sizeof(unsigned long) // incarnation_#
                        + sizeof(unsigned) // pb_ml_size
                        + piggybackMembershipListMsg.size() // pb_ml
                        + sizeof(unsigned) // pb_fl_size
                        + piggybackFailList.size(); // pb_fl
    std::cout << "msgSize: " << msgSize << std::endl;
    std::cout << "ACK: " << MsgTypes::ACK << std::endl;
    char* msg = new char[msgSize];
    auto msgStart = msg;
    MsgTypes::Types type = MsgTypes::ACK;
    MP1Node::copyMsg(msgStart, type);
    MP1Node::copyMsg(msgStart, this->source);
    MP1Node::copyMsg(msgStart, this->destination);
    MP1Node::copyMsg(msgStart, this->protocol_period);
    MP1Node::copyMsg(msgStart, this->incarnation);
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
    cout << "In AckMessage::encode()..." << endl;
    cout << "msgSize: " << msgSize << endl;
    cout << "msg: " << rtn << endl;
#endif
    return rtn;
}

void AckMessage::onReceiveHandler(MP1Node& state) {
    // TODO: Fill this in.
#ifdef DEBUGLOG
    cout << "In AckMessage::onReceiveHandler..." << endl;
#endif
}

void AckMessage::printMsg() {
    cout << "########## AckMessage ##########" << endl;
    cout << "# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "# id: " << getId() << endl;
    cout << "# source: " << source.getAddress() << endl;
    cout << "# destination: " << destination.getAddress() << endl;
    cout << "# protocol_period: " << protocol_period << endl;
    cout << "# incarnation: " << incarnation << endl;
    cout << "# piggybackMembershipList: " << endl;
    for(auto& entry : piggybackMembershipList) {
        cout << "# " << entry << endl;
    }
    cout << "# piggybackFailList: " << endl;
    for(auto& entry : piggybackFailList) {
        cout << "# " << entry << endl;
    }
    cout << "########## ########## #########" << endl;
}
