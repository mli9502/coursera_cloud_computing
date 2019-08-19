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
    cout << "\t########## PingMessage ##########" << endl;
    cout << "\t# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "\t# id: " << getId() << endl;
    cout << "\t# source: " << source.getAddress() << endl;
    cout << "\t# destination: " << destination.getAddress() << endl;
    cout << "\t# protocol_period: " << protocol_period << endl;
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

bool PingMessage::onReceiveHandler(MP1Node& node) {
    node.processPiggybackLists(piggybackMembershipList, piggybackFailList);

    vector<MembershipListEntry> respPiggybackMembershipListEntries = node.getMembershipList().getTopK(node.K, node.getMaxPiggybackCnt());
    vector<FailListEntry> respPiggybackFailListEntries = node.getFailList().getTopK(node.K, node.getMaxPiggybackCnt());

    Address newSource = node.getMemberNode()->addr;
    Address newDestination = this->source;
    // Send Ack message back.
    shared_ptr<BaseMessage> ackMsg = make_shared<AckMessage>(MsgTypes::ACK,
                                                            newSource,
                                                            newDestination,
                                                            this->protocol_period, // Note that in here, we take the protocolPeriod from the Ping msg we received.
                                                            node.getIncarnationNum(),
                                                            respPiggybackMembershipListEntries,
                                                            respPiggybackFailListEntries);

    vector<char> encodedAck = ackMsg->encode();
    int sizeSent = node.getEmulNet()->ENsend(&newSource, &newDestination, encodedAck.data(), encodedAck.size());
    if(sizeSent == 0) {
        cerr << "Ping msg from: " << newSource.getAddress()
                << " to: " << newDestination.getAddress() << " is not sent." << endl;
    }
    return true;
}