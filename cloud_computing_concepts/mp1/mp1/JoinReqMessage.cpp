#include "JoinReqMessage.h"

#include "JoinRespMessage.h"

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

// JoinReq message will only be received by coordinator.
// @state: The MP1Node that receives this message.
// On receive, the membershipList and failList of node (which should be the coordinator), 
// will be taken and insert into JoinResp message to send to the newly joined node.
bool JoinReqMessage::onReceiveHandler(MP1Node& node) {
#ifdef DEBUGLOG
    cout << "In JoinReqMessage::onReceiveHandler at Node: " << destination.getAddress() << endl;
#endif
    node.getMembershipList().appendEntry(source);
    // Construct a JoinResp message.
    // The destination in the decoded message is now source, and source is now destination.
    Address newSource = destination;
    Address newDestination = source;
    shared_ptr<BaseMessage> respMsg = make_shared<JoinRespMessage>(MsgTypes::Types::JOINRESP, 
                                                                    newSource, 
                                                                    newDestination, 
                                                                    node.getMembershipList().getFirstK(JoinRespMessage::MAX_LIST_ENTRY), 
                                                                    node.getFailList().getFirstK(JoinRespMessage::MAX_LIST_ENTRY));
    
    vector<char> encodedResp = respMsg->encode();
    int sizeSent = node.getEmulNet()->ENsend(&newSource, &newDestination, encodedResp.data(), encodedResp.size());
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

void JoinReqMessage::printMsg() {
    cout << "########## JoinReqMessage ##########" << endl;
    cout << "# message type: " << MsgTypes::to_string(msgType) << endl;
    cout << "# id: " << getId() << endl;
    cout << "# source: " << source.getAddress() << endl;
    cout << "# destination: " << destination.getAddress() << endl;
    cout << "########## ########## #########" << endl;
}