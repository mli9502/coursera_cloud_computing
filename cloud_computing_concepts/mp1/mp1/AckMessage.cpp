#include "AckMessage.h"

#include <bitset>

string AckMessage::getId() {
    return destination.getAddress() + "|" + source.getAddress() + "|" + to_string(protocol_period);
}

void AckMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    copyObj(msgPtr, this->msgType);
    copyObj(msgPtr, this->source);
    copyObj(msgPtr, this->destination);
    copyObj(msgPtr, this->protocol_period);
    copyObj(msgPtr, this->incarnation);
    decodePiggybackLists(msgPtr);
}

// pb: piggyback
// ml: membership_list
// fl: fail_list
// MsgType|src_address|dest_address|protocol_period|incarnation_#|pb_ml_size|pb_ml|pb_fl_size|pb_fl|
vector<char> AckMessage::encode() {
    unsigned msgSizeBeforePiggyback = sizeof(MsgTypes::Types) // MsgType
                                        + sizeof(Address) // src_address
                                        + sizeof(Address) // dest_address
                                        + sizeof(unsigned long) // protocol_period
                                        + sizeof(unsigned long); // incarnation_#

    vector<char> msg(msgSizeBeforePiggyback);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::ACK;
    copyMsg(msgStart, type);
    copyMsg(msgStart, this->source);
    copyMsg(msgStart, this->destination);
    copyMsg(msgStart, this->protocol_period);
    copyMsg(msgStart, this->incarnation);

    encodeAndAppendPiggybackLists(msg);

    return msg;
}

bool AckMessage::onReceiveHandler(MP1Node& node) {
    // TODO: Fill this in.
#ifdef DEBUGLOG
    cout << "In AckMessage::onReceiveHandler at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif
    node.getPingMap().print();
    // First check if id is in pingMap.
    if(node.getPingMap().contains(getId())) {
#ifdef DEBUGLOG
        cout << "Found id: " << getId() << " in pingMap for node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif
    }
    return true;
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
