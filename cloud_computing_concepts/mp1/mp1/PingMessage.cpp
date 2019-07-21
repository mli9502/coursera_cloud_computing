#include "PingMessage.h"
#include "AckMessage.h"

string PingMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    copyObj(msgPtr, this->msgType);
    copyObj(msgPtr, this->source);
    copyObj(msgPtr, this->destination);
    copyObj(msgPtr, this->protocol_period);
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
    copyMsg(msgStart, type);
    copyMsg(msgStart, this->source);
    copyMsg(msgStart, this->destination);
    copyMsg(msgStart, this->protocol_period);

    encodeAndAppendPiggybackLists(msg);

    return msg;
}

void PingMessage::printMsg() {
    cout << "########## PingMessage ##########" << endl;
    cout << "# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "# id: " << getId() << endl;
    cout << "# source: " << source.getAddress() << endl;
    cout << "# destination: " << destination.getAddress() << endl;
    cout << "# protocol_period: " << protocol_period << endl;
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

bool PingMessage::onReceiveHandler(MP1Node& node) {
    // TODO: fill this in.
#ifdef DEBUGLOG
    cout << "In PingMessage::onReceiveHandler at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif

    // Update the membershipList and failList in current node using the piggyback lists.
    // TODO: Do we have to handle the case where we receive an entry in membership list, 
    //       but this address is already marked fail in current node's fail list?
    // TODO:
    // Two cases need to be considered here:
    // - Node is in the received membershipList, and also in curr node's failList.
    // - Node is in the received failList, and also in curr node's membershipList.
    // TODO: When we send ACK message back, we need to decode the ID from the source msg.
    //          And use the protocolPeriodCnt from the source msg to construct the ID of the ACK msg.
    //          By doing this, we can make sure that this ID matches at the source side.
    for(auto& entry : piggybackMembershipList) {
        node.getMembershipList().insertEntryAtRandom(entry);
    }
    for(auto& entry : piggybackFailList) {
        node.getFailList().insertEntry(entry);
    }

    vector<MembershipListEntry> respPiggybackMembershipListEntries = node.getMembershipList().getTopK(node.K, node.getMaxPiggybackCnt());
    vector<FailListEntry> respPiggybackFailListEntries = node.getFailList().getTopK(node.K, node.getMaxPiggybackCnt());

    Address newSource = node.getMemberNode()->addr;
    Address newDestination = this->source;
    // Send Ack message back.
    shared_ptr<BaseMessage> ackMsg = make_shared<AckMessage>(MsgTypes::ACK,
                                                            newSource,
                                                            newDestination,
                                                            node.getProtocolPeriod(),
                                                            node.getIncarnationNum(),
                                                            respPiggybackMembershipListEntries,
                                                            respPiggybackFailListEntries);

    vector<char> encodedPing = ackMsg->encode();
    int sizeSent = node.getEmulNet()->ENsend(&newSource, &newDestination, encodedPing.data(), encodedPing.size());
    if(sizeSent == 0) {
#ifdef DEBUGLOG
        cout << "sizeSent is 0, msg is not sent..." << endl;
#endif
    } else {
#ifdef DEBUGLOG
        cout << "sizeSent is " << sizeSent << endl;
#endif
    }
    return true;
}