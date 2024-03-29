#include "PingReqMessage.h"

#include "PingMessage.h"

#include <memory>

string PingReqMessage::getId() {
    // Note that in here, we ignore the destination address, because when we receive ACK for this message, we receive it from route, not destination.
    return source.getAddress() + "|" + route.getAddress() + "|" + to_string(protocol_period);
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
    
    return msg;
}

void PingReqMessage::printMsg() {
    cout << "\t########## PingReqMessage ##########" << endl;
    cout << "\t# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "\t# id: " << getId() << endl;
    cout << "\t# source: " << source.getAddress() << endl;
    cout << "\t# route: " << route.getAddress() << endl;
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

bool PingReqMessage::onReceiveHandler(MP1Node& node) {
    node.processPiggybackLists(this->piggybackMembershipList, this->piggybackFailList);
    
    vector<MembershipListEntry> respPiggybackMembershipListEntries = node.getMembershipList().getTopK(node.K, node.getMaxPiggybackCnt());
    vector<FailListEntry> respPiggybackFailListEntries = node.getFailList().getTopK(node.K, node.getMaxPiggybackCnt());

    // Send Ping msg to destination node, and record the ID of this Ping msg and source into pingReqPing map.
    shared_ptr<BaseMessage> pingMsg = make_shared<PingMessage>(MsgTypes::PING, 
                                                                route, 
                                                                destination, 
                                                                protocol_period, 
                                                                respPiggybackMembershipListEntries, 
                                                                respPiggybackFailListEntries);
    node.getPingReqPingMap().insert(pingMsg->getId(), source);
    vector<char> encodedPing = pingMsg->encode();
    int sizeSent = node.getEmulNet()->ENsend(&route, &destination, encodedPing.data(), encodedPing.size());
    if(sizeSent == 0) {
        cerr << "Ping msg sending from route: " << route.getAddress()
                << " to: " << destination.getAddress() << " is not sent." << endl;
    }
    return true;
}

