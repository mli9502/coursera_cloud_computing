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
    node.processPiggybackLists(piggybackMembershipList, piggybackFailList);
    cerr << "Before JoinRespMessage::onReceiveHandler return..." << endl;
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