#include "JoinReqMessage.h"

string JoinReqMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress();
}

// MsgType|src_address|dest_address|
vector<char> JoinReqMessage::encode() {
    unsigned msgSize = sizeof(MsgTypes::Types) // MsgType
                                    + sizeof(Address) // src_address
                                    + sizeof(Address); // dest_address

    vector<char> msg(msgSize);
    char* msgStart = &msg[0];

    MsgTypes::Types type = MsgTypes::JOINREQ;
    copyMsg(msgStart, type);
    copyMsg(msgStart, this->source);
    copyMsg(msgStart, this->destination);

    return msg;
}

void JoinReqMessage::decode(const vector<char>& msg) {
    char const* msgPtr = &(msg[0]);
    copyObj(msgPtr, this->msgType);
    copyObj(msgPtr, this->source);
    copyObj(msgPtr, this->destination);
}

void JoinReqMessage::onReceiveHandler(MP1Node& state) {
    // TODO: Fill this in.
#ifdef DEBUGLOG
    cout << "In JoinReqMessage::onReceiveHandler..." << endl;
#endif
}

void JoinReqMessage::printMsg() {
    cout << "########## JoinReqMessage ##########" << endl;
    cout << "# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "# id: " << getId() << endl;
    cout << "# source: " << source.getAddress() << endl;
    cout << "# destination: " << destination.getAddress() << endl;
    cout << "########## ########## #########" << endl;
}