#include "JoinRespMessage.h"

#include "PingMessage.h"

const unsigned JoinRespMessage::MAX_LIST_ENTRY = 5;

string JoinRespMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress();
}

// MsgType|src_address|dest_address|
vector<char> JoinRespMessage::encode() {
    unsigned msgSize = sizeof(MsgTypes::Types) // MsgType
                        + sizeof(Address) // src_address
                        + sizeof(Address); // dest_address

    vector<char> msg(msgSize);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::JOINRESP;
    copyMsg(msgStart, type);
    copyMsg(msgStart, this->source);
    copyMsg(msgStart, this->destination);

    encodeAndAppendPiggybackLists(msg);

    return msg;
}

void JoinRespMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    copyObj(msgPtr, this->msgType);
    copyObj(msgPtr, this->source);
    copyObj(msgPtr, this->destination);
    decodePiggybackLists(msgPtr);
}

bool JoinRespMessage::onReceiveHandler(MP1Node& node) {
#ifdef DEBUGLOG
    cout << "In JoinRespMessage::onReceiveHandler at node: " << node.getMemberNode()->addr.getAddress() << endl;
#endif

    // Insert all the MembershipListNode and FailListNode.
    for(auto& entry : piggybackMembershipList) {
        node.getMembershipList().appendEntry(entry);
    }
    for(auto& entry : piggybackFailList) {
        node.getFailList().insertEntry(entry);
    }
    
    // From the membership list, select K nodes to send Ping.
    // NOTE: Note that when constructing Ping msg, we don't actually need the message that triggers this.
    //       We only need to have the source node.
    Address newSource = node.getMemberNode()->addr;
    
    vector<MembershipListEntry> respPiggybackMembershipListEntries = node.getMembershipList().getTopK(node.K, node.getMaxPiggybackCnt());
    vector<FailListEntry> respPiggybackFailListEntries = node.getFailList().getTopK(node.K, node.getMaxPiggybackCnt());

    unsigned long currProtocolPeriod = node.getProtocolPeriod();

    // Select a pingTarget from membershipList to send Ping msg.
    MembershipListEntry pingTarget;
    bool rc = node.getMembershipList().getPingTarget(pingTarget, node.getMemberNode()->addr);
    if(!rc) {
#ifdef DEBUGLOG
        cout << "getPingTarget failed at node: " << newSource.getAddress() << " ..." << endl;
#endif
        return false;
    }
#ifdef DEBUGLOG
        cout << "Selected Ping target: " << pingTarget.getAddress() << " at Node: " << newSource.getAddress() << endl;
        cout << "Sending Ping msg to: " << pingTarget.getAddress() << endl;
#endif

    shared_ptr<BaseMessage> pingMsg = make_shared<PingMessage>(MsgTypes::Types::PING,
                                                                newSource,
                                                                pingTarget.addr,
                                                                currProtocolPeriod,
                                                                respPiggybackMembershipListEntries,
                                                                respPiggybackFailListEntries);

    vector<char> encodedPing = pingMsg->encode();
    int sizeSent = node.getEmulNet()->ENsend(&newSource, &(pingTarget.addr), encodedPing.data(), encodedPing.size());
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

void JoinRespMessage::printMsg() {
    cout << "########## JoinRespMessage ##########" << endl;
    cout << "# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "# id: " << getId() << endl;
    cout << "# source: " << source.getAddress() << endl;
    cout << "# destination: " << destination.getAddress() << endl;
    cout << "# coordinator membershipList: " << endl;
    for(auto& entry : piggybackMembershipList) {
        cout << "# " << entry << endl;
    }
    cout << "# coordinator failList: " << endl;
    for(auto& entry : piggybackFailList) {
        cout << "# " << entry << endl;
    }
    cout << "########## ########## #########" << endl;
}