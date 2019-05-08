#include "AckMessage.h"

#include <bitset>

string AckMessage::getId() {
    return destination.getAddress() + "|" + source.getAddress() + "|" + to_string(protocol_period);
}

void AckMessage::decode(const vector<char>& msg) {
    std::cout << "----------------------------" << std::endl;
    for(unsigned i = 0; i < msg.size(); i ++) {
        std::cout << std::bitset<8>(unsigned(msg[i])) << std::endl; 
    }
    char* msgPtr = const_cast<char*>(&(msg[0]));
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
    unsigned startIdx = msgPtr - &msg[0];
    char* tmpPtr = const_cast<char*>(&msg[0]) + startIdx;
    unsigned numMembershipListEntry;
    MP1Node::copyObj(tmpPtr, numMembershipListEntry);
    unsigned totalMembershipListSize = sizeof(unsigned) + numMembershipListEntry * MembershipListEntry::getEntrySize();
    MembershipList::decodeTopKMsg(vector<char>(msg.begin() + startIdx, msg.begin() + startIdx + totalMembershipListSize), this->piggybackMembershipList);
    
    tmpPtr += totalMembershipListSize;
    startIdx += totalMembershipListSize;
    unsigned numFailListEntry;
    MP1Node::copyObj(tmpPtr, numFailListEntry);
    // sizeof(unsigned): The leading size.
    unsigned totalFailListSize = sizeof(unsigned) + numFailListEntry * FailListEntry::getEntrySize();
    FailList::decodeTopKMsg(vector<char>(msg.begin() + startIdx, msg.begin() + startIdx + totalFailListSize), this->piggybackFailList);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|dest_address|protocol_period|incarnation_#|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
vector<char> AckMessage::encode() {
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

    vector<char> msg(msgSize);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::ACK;
    MP1Node::copyMsg(msgStart, type);
    MP1Node::copyMsg(msgStart, this->source);
    MP1Node::copyMsg(msgStart, this->destination);
    MP1Node::copyMsg(msgStart, this->protocol_period);
    MP1Node::copyMsg(msgStart, this->incarnation);
    unsigned pbMlSize = this->piggybackMembershipList.size();
    MP1Node::copyMsg(msgStart, pbMlSize);
    memcpy(msgStart, &piggybackMembershipListMsg[0], piggybackMembershipListMsg.size());
    msgStart += piggybackMembershipListMsg.size();
    unsigned pbFlSize = this->piggybackFailList.size();
    MP1Node::copyMsg(msgStart, pbFlSize);
    memcpy(msgStart, &piggybackFailListMsg[0], piggybackFailListMsg.size());
    msgStart += piggybackFailListMsg.size();

#ifdef DEBUGLOG
    cout << "In AckMessage::encode()..." << endl;
    cout << "msgSize: " << msgSize << endl;
#endif
    return msg;
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
