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
    auto nodeAddr = node.getMemberNode()->addr.getAddress();
    // First handle the piggyback lists.
    node.processPiggybackLists(this->piggybackMembershipList, this->piggybackFailList);

    // Then, check if id is in pingMap.
    if(node.getPingMap().contains(getId())) {
#ifdef DEBUGLOG
        cout << "Found id: " << getId() << " in pingMap at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif
        node.getPingMap().erase(getId());
        node.setAckReceived();
    } else if(node.getPingReqMap().contains(getId())) {
#ifdef DEBUGLOG
        cout << "Found id: " << getId() << " in pingReqMap at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif
        node.getPingReqMap().erase(getId());
        node.setAckReceived();
    } else if(node.getPingReqPingMap().contains(getId())) {
        // Route ACK back to source.
#ifdef DEBUGLOG
        cout << "Found id: " << getId() << " in pingReqPingMap at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif
        vector<MembershipListEntry> respPiggybackMembershipListEntries = node.getMembershipList().getTopK(node.K, node.getMaxPiggybackCnt());
        vector<FailListEntry> respPiggybackFailListEntries = node.getFailList().getTopK(node.K, node.getMaxPiggybackCnt());

        Address pingReqSource = *(node.getPingReqPingMap().get(getId()));
        shared_ptr<BaseMessage> ackMsg = make_shared<AckMessage>(MsgTypes::ACK,
                                                                    node.getMemberNode()->addr,
                                                                    pingReqSource,
                                                                    this->protocol_period,
                                                                    this->incarnation,
                                                                    respPiggybackMembershipListEntries,
                                                                    respPiggybackFailListEntries);
        vector<char> encodedAck = ackMsg->encode();
        int sizeSent = node.getEmulNet()->ENsend(&node.getMemberNode()->addr, &pingReqSource, encodedAck.data(), encodedAck.size());
        if(sizeSent == 0) {
    #ifdef DEBUGLOG
            cout << "sizeSent is 0, msg is not sent..." << endl;
    #endif
        }   
    }

    return true;
}

void AckMessage::printMsg() {
    cout << "\t########## AckMessage ##########" << endl;
    cout << "\t# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "\t# id: " << getId() << endl;
    cout << "\t# source: " << source.getAddress() << endl;
    cout << "\t# destination: " << destination.getAddress() << endl;
    cout << "\t# protocol_period: " << protocol_period << endl;
    cout << "\t# incarnation: " << incarnation << endl;
    cout << "\t# piggybackMembershipList: " << endl;
    for(auto& entry : piggybackMembershipList) {
        cout << "\t# " << entry << endl;
    }
    cout << "\t# piggybackFailList: " << endl;
    for(auto& entry : piggybackFailList) {
        cout << "\t# " << entry << endl;
    }
    cout << "\t########## ########## #########" << endl;
}
