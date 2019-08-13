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
    node.processPiggybackLists(piggybackMembershipList, piggybackFailList);
    return true;
}

void JoinRespMessage::printMsg() {
    cerr << "\t########## JoinRespMessage ##########" << endl;
    cerr << "\t# message type: " << MsgTypes::to_string(msgType) << endl;
    cerr << "\t# id: " << getId() << endl;
    cerr << "\t# source: " << source.getAddress() << endl;
    cerr << "\t# destination: " << destination.getAddress() << endl;
    cerr << "\t# coordinator membershipList: " << endl;
    for(auto& entry : piggybackMembershipList) {
        cerr << "\t# " << entry << endl;
    }
    cerr << "\t# coordinator failList: " << endl;
    for(auto& entry : piggybackFailList) {
        cerr << "\t# " << entry << endl;
    }
    cerr << "\t########## ########## #########" << endl;
}