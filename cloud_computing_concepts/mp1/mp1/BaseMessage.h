#ifndef _BASE_MESSAGE_H_
#define _BASE_MESSAGE_H_

#include "stdincludes.h"

#include "MP1Node.h"

class BaseMessage {
public:
    BaseMessage() {};
    // ctor in order to encode.
    BaseMessage(MsgTypes::Types msgType,
                Address source,
                Address destination,
                unsigned long protocol_period,
                const vector<MembershipListEntry>& piggybackMembershipList,
                const vector<FailListEntry>& piggybackFailList) :
                msgType(msgType),
                source(source),
                destination(destination),
                protocol_period(protocol_period),
                piggybackMembershipList(piggybackMembershipList),
                piggybackFailList(piggybackFailList) {}

    virtual ~BaseMessage() {};

    MsgTypes::Types getMsgType() {
        return msgType;
    }
    Address getSrc() {
        return source;
    }
    Address getDest() {
        return destination;
    }
    const vector<MembershipListEntry>& getMembershipListInPiggyback() {
        return piggybackMembershipList;
    }
    const vector<FailListEntry>& getFailListInPiggyback() {
        return piggybackFailList;
    } 
    // JOIN_REQ: source|destination
    // JOIN_RESP: source|destination
    // PING: source|destination|protocol_period.
    // PING_REQ: source|route|destination|protocol_period.
    // ACK: destination|source|protocol_period.
    virtual string getId() = 0;
    virtual void decode(const vector<char>& msg) = 0;
    virtual vector<char> encode() = 0;
    // This method implements what we should do when we receive a type of message.
    // TODO: @7/2/2019: Update this to return bool, and add getters for private members in MP1Node.
    virtual shared_ptr<BaseMessage> onReceiveHandler(MP1Node& node) = 0;
    virtual void printMsg() = 0;

protected:
    MsgTypes::Types msgType;
    Address source;
    Address destination;
    unsigned long protocol_period;
    vector<MembershipListEntry> piggybackMembershipList;
    vector<FailListEntry> piggybackFailList;

    void decodePiggybackLists(char const*& msgPtr) {
        MembershipList::decodeTopKMsg(msgPtr, this->piggybackMembershipList);
        FailList::decodeTopKMsg(msgPtr, this->piggybackFailList);
    }
    /**
     * Encode the piggybackMembershipList and piggybackFailList and insert into the msg.
     * Note that the size of msg will be extended by this method.
     */
    void encodeAndAppendPiggybackLists(vector<char>& msg) {
        unsigned prevMsgSize = msg.size();
        vector<char> piggybackMembershipListMsg = MembershipList::encodeTopKMsg(this->piggybackMembershipList);
        vector<char> piggybackFailListMsg = FailList::encodeTopKMsg(this->piggybackFailList);
        // prev_msg_size + (pb_ml_entries + pb_ml) + (pb_fl_entries + pb_fl).
        unsigned newMsgSize = prevMsgSize + 
                                piggybackMembershipListMsg.size() + 
                                piggybackFailListMsg.size();
        msg.resize(newMsgSize);
        char* msgPtr = &(msg[0]) + prevMsgSize;
        // Insert the membership_list.size() + membership_list.
        memcpy(msgPtr, &piggybackMembershipListMsg[0], piggybackMembershipListMsg.size());
        msgPtr += piggybackMembershipListMsg.size();
        // Insert the fail_list.size() + fail_list.
        memcpy(msgPtr, &piggybackFailListMsg[0], piggybackFailListMsg.size());
        msgPtr += piggybackFailListMsg.size();
    }
};

#endif