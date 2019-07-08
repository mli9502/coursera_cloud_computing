/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

#include "JoinReqMessage.h"

#include "MessageDecoder.h"

long FailListEntry::MAX_LIVE_TIME = 5000;

ostream& operator<<(ostream& os, const MembershipListEntry& rhs) {
	os << const_cast<MembershipListEntry&>(rhs).getAddress() << ": [piggybackCnt " << rhs.piggybackCnt << "], " << 
                                                                "[type " << rhs.getMemberTypeStr() << "], " << 
                                                                "[incarnationNum " << rhs.incarnationNum << "]";
	return os;
}

ostream& operator<<(ostream& os, const FailListEntry& rhs) {
    os << const_cast<FailListEntry&>(rhs).getAddress() << ": FAIL";
    return os;
}

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */
const long MP1Node::PING_TIMEOUT = 2;
const long MP1Node::PING_REQ_TIMEOUT = 2 * MP1Node::PING_TIMEOUT;
const long MP1Node::PROTOCOL_PERIOD = MP1Node::PING_TIMEOUT + MP1Node::PING_REQ_TIMEOUT + 2;

const int MP1Node::K = 5;
const int MP1Node::lambda = 5;
// This is the number of targets we selected to send PING_REQ each period.
// TODO: Not quite sure 5 should be the correct number. May need to modify this.
const int MP1Node::NUM_PING_REQ_TARGETS = 5;

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) : 
                    incarnationNum(0),
                    failList(),
                    membershipList(),
                    protocolPeriodCnt(0),
                    pingMap(PING_TIMEOUT),
                    pingReqMap(PING_REQ_TIMEOUT),
                    pingReqPingMap(PING_REQ_TIMEOUT) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
    // Insert the current node into membership list.
#ifdef DEBUGLOG
    cout << "Adding " << memberNode->addr.getAddress() << " to the membershipList of itself." << endl;
#endif
    this->membershipList.appendEntry(memberNode->addr);
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * Get the max count for piggy-back of an entry.
 * This is calculated by (lambda * log(N)).
 */
int MP1Node::getMaxPiggybackCnt() {
    return lambda * std::log2(par->MAX_NNB);
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
    // This enqueue actually push "buffer" into "env"...
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        JoinReqMessage joinReqMsg { MsgTypes::JOINREQ, memberNode->addr, *joinaddr };
        vector<char> msg = joinReqMsg.encode();

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, msg.data(), msg.size());
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size) {
	/*
	 * Your code goes here
	 */

    MsgTypes::Types msgType = MessageDecoder::getTypeFromMsg(data, size);
    cerr << "Received msg type: " << MsgTypes::to_string(msgType) << " at Node: " << this->memberNode->addr.getAddress() << endl;
    shared_ptr<BaseMessage> msg = MessageDecoder::decode(data, size);
    if(!msg) {
#ifdef DEBUGLOG
        cout << "Getting nullptr as decoded msg! Failed to decode message..." << endl;
#endif
        return false;
    }
#ifdef DEBUGLOG
    msg->printMsg();
#endif
    return msg->onReceiveHandler(*this);
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

MembershipList& MP1Node::getMembershipList() {
    return this->membershipList;
}

FailList& MP1Node::getFailList() {
    return this->failList;
}

EmulNet* MP1Node::getEmulNet() {
    return this->emulNet;
}

// A msg id consists of id_addr and id_period_cnt.
// id_addr: The address of the node that initiate the initial PING message.
// id_period_cnt: The period counter at the node that initiate the initial PING message.

// MsgTypes|id_addr|id_period_cnt|from_address|to_address|membership_list_top_K_msg|fail_list_top_K_msg
pair<unsigned, char*> MP1Node::genPingMsg(MembershipListEntry to, Address idAddr, unsigned long idPeriodCnt) {
    // First gen top K msg from membership list and fail list.
    // pair<unsigned, char*> membershipListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // pair<unsigned, char*> failListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // unsigned msgSize = sizeof(MsgTypes) + sizeof(Address) + sizeof(unsigned long) + 2 * sizeof(Address) + membershipListMsg.first + failListMsg.first;
    // char* msg = new char[msgSize];
    // auto msgStart = msg;
    // MsgTypes type = MsgTypes::PING;
    // copyMsg(msgStart, type);
    // copyMsg(msgStart, idAddr);
    // copyMsg(msgStart, idPeriodCnt);
    // copyMsg(msgStart, this->memberNode->addr);
    // copyMsg(msgStart, to.addr);
    // memcpy(msgStart, membershipListMsg.second, membershipListMsg.first);
    // msgStart += membershipListMsg.first;
    // memcpy(msgStart, failListMsg.second, failListMsg.first);
    // msgStart += failListMsg.first;
    // return {msgSize, msg};
    return {0, 0};
}
// MsgTypes|id_addr|id_period_cnt|from_address|to_address|req_address|membership_list_top_K_msg|fail_list_top_K_msg
pair<unsigned, char*> MP1Node::genPingReqMsg(MembershipListEntry to, MembershipListEntry req, Address idAddr, unsigned long idPeriodCnt) {
    // First gen top K msg from membership list and fail list.
    // pair<unsigned, char*> membershipListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // pair<unsigned, char*> failListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // unsigned msgSize = sizeof(MsgTypes) + sizeof(Address) + sizeof(unsigned long) + 3 * sizeof(Address) + membershipListMsg.first + failListMsg.first;
    // char* msg = new char[msgSize];
    // auto msgStart = msg;
    // MsgTypes type = MsgTypes::PING_REQ;
    // copyMsg(msgStart, type);
    // copyMsg(msgStart, idAddr);
    // copyMsg(msgStart, idPeriodCnt);
    // copyMsg(msgStart, this->memberNode->addr);
    // copyMsg(msgStart, to.addr);
    // copyMsg(msgStart, req.addr);
    // memcpy(msgStart, membershipListMsg.second, membershipListMsg.first);
    // msgStart += membershipListMsg.first;
    // memcpy(msgStart, failListMsg.second, failListMsg.first);
    // msgStart += failListMsg.first;
    // return {msgSize, msg};
    return {0, 0};
}
// MsgTypes|id_addr|id_period_cnt|from_address|to_address|membership_list_top_K_msg|fail_list_top_K_msg
pair<unsigned, char*> MP1Node::genAckMsg(MembershipListEntry to, Address idAddr, unsigned long idPeriodCnt) {
    // First gen top K msg from membership list and fail list.
    // pair<unsigned, char*> membershipListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // pair<unsigned, char*> failListMsg = membershipList.genTopKMsg(MP1Node::K, getMaxPiggybackCnt());
    // unsigned msgSize = sizeof(MsgTypes) + sizeof(Address) + sizeof(unsigned long) + 2 * sizeof(Address) + membershipListMsg.first + failListMsg.first;
    // char* msg = new char[msgSize];
    // auto msgStart = msg;
    // MsgTypes type = MsgTypes::ACK;
    // copyMsg(msgStart, type);
    // copyMsg(msgStart, idAddr);
    // copyMsg(msgStart, idPeriodCnt);
    // copyMsg(msgStart, this->memberNode->addr);
    // copyMsg(msgStart, to.addr);
    // memcpy(msgStart, membershipListMsg.second, membershipListMsg.first);
    // msgStart += membershipListMsg.first;
    // memcpy(msgStart, failListMsg.second, failListMsg.first);
    // msgStart += failListMsg.first;
    // return {msgSize, msg};
    return {0, 0};
}
// TODO: Need to fill in this next.
// Use MembershipList::decodeTopKMsg and FailList::decodeTopKMsg to decode top K msgs.
void MP1Node::decodePingMsg(char* msg, 
                            Address& idAddr, 
                            unsigned long& idPeriodCnt,
                            Address& fromAddr, 
                            Address& toAddr,
                            vector<MembershipListEntry>& membershipListTopK,
                            vector<FailListEntry>& failListTopK) {

}

// Get message type from message.
MsgTypes::Types MsgHelper::getMsgType(char* data) {
    MessageHdr* tmp = (MessageHdr*)malloc(sizeof(MessageHdr));
    memcpy(tmp, data, sizeof(MessageHdr));
    MsgTypes::Types rtn = tmp->msgType;
    free(tmp);
    return rtn;
} 

string MsgHelper::getMsgTypeStr(MsgTypes::Types mt) {
    return MsgTypes::to_string(mt);
}