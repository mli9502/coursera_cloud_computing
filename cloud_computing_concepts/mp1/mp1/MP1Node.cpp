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
// TODO: Need to update this PING_TIMEOUT. 
// 2 is two small...
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
                    protocolPeriodCnt(0),
                    protocolPeriodLocalCounter(-1),
                    ackReceived(false),
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
   // TODO: Do we need to add stuff in here?
   return 0;
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
    shared_ptr<BaseMessage> msg = MessageDecoder::decode(data, size);
    if(!msg) {
#ifdef DEBUGLOG
        cout << "Getting nullptr as decoded msg! Failed to decode message..." << endl;
#endif
        return false;
    }
    cerr << "Received msg type: " << MsgTypes::to_string(msgType) 
        << " at Node: " << this->memberNode->addr.getAddress() 
        << " from Node: " << msg->getSrc().getAddress() << endl;
    msg->printMsg();
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
    this->updatePeriod();
    // TODO: @7/20/2019: When receive JoinResp message, we probably don't need to send out Ack message.
    //                   We can wait until the start of the next protocolPeriod.
    // If this is the start of a protocolPeriod, start sending Ping message.
    if(this->protocolPeriodLocalCounter == 0) {
        // TODO: In here, we need to clear the entries with previous protocolPeriod out of the TimeoutMap.
        // In fact, we can just clear the pingMap.
        // Can also clear pingReqMap, but we can't clear pingReqPing map since this map does not follow local protocol period.
        cerr << "Before sending out ping message at protocol period: " << protocolPeriodCnt << endl;
        this->sendPingMsg();
    } else if(this->protocolPeriodLocalCounter == PING_TIMEOUT && !this->ackReceived) {
        cerr << "Before sending out pingReq message at protocol period: " << protocolPeriodCnt << endl;
        this->sendPingReqMsg();
    } else if(this->protocolPeriodLocalCounter == PROTOCOL_PERIOD - 1) {
        cerr << "At the end of protocol period: " << protocolPeriodCnt << endl;
        // If the ack is not received, need to either mark this node as suspeced, or mark it as failed.
        if(!this->ackReceived) {
            cerr << "Before handleFailedPing at node: " << getMemberNode()->addr.getAddress() << endl;
            handleFailedPing();
        } else {
            cerr << "Before handleSuccessPing at node: " << getMemberNode()->addr.getAddress() << endl;
            handleSuccessPing();
        }
        dumpMembershipAndFailLists();
        // Reset ackReceived for next protocol period.
        this->ackReceived = false;
    }
    // If this is the end of a protocolPeriod, check to see if Ack message has been received.
    return;
}

void MP1Node::handleSuccessPing() {
    auto localEntryPtr = membershipList.containsNode(*currPingTarget);
    if(!localEntryPtr) {
        cerr << "handleSuccessPing: pingTarget: " << currPingTarget->getAddress() << " is not found in membershipList. Might alreay been marked as failed." << endl;
    } else {
        if(localEntryPtr->getType() == SUSPECT) {
            cerr << "Mark pingTarget: " << currPingTarget->getAddress() << " as ALIVE." << endl;
            localEntryPtr->markAsAlive();
        }
    }
}

void MP1Node::handleFailedPing() {
    if(!this->currPingTarget) {
        cerr << "No pingTarget was selected. No need to handle failed ping..." << endl;
    }
    // Check if this node is in membership list.
    auto localEntryPtr = membershipList.containsNode(*currPingTarget);
    if(!localEntryPtr) {
        cerr << "handleFailedPing: pingTarget: " << currPingTarget->getAddress() << " is not found in membershipList. Might alreay been marked as failed." << endl;
    } else {
        if(localEntryPtr->getType() == ALIVE) {
            cerr << "Mark pingTarget: " << currPingTarget->getAddress() << " as SUSPECTED." << endl;
            localEntryPtr->markAsSuspected();
        } else {
            cerr << "Mark pingTarget: " << currPingTarget->getAddress() << " as FAILED." << endl;
            log->logNodeRemove(&(this->getMemberNode()->addr), currPingTarget.get());
            failList.insertEntry(*localEntryPtr);
            membershipList.removeEntry(localEntryPtr->getAddress());
        }
    }
}

bool MP1Node::sendPingMsg() {
    Address source = getMemberNode()->addr;

    vector<MembershipListEntry> piggybackMembershipListEntries = getMembershipList().getTopK(K, getMaxPiggybackCnt());
    vector<FailListEntry> piggybackFailListEntries = getFailList().getTopK(K, getMaxPiggybackCnt());

    unsigned long currProtocolPeriod = getProtocolPeriod();

    MembershipListEntry pingTarget;
    // Select a pingTarget from membershipList to send Ping msg.
    bool rc = getMembershipList().getPingTarget(pingTarget, source);
    if(!rc) {
        cerr << "getPingTarget failed at node: " << source.getAddress() << " ..." << endl;
        return false;
    }
    this->currPingTarget = make_shared<Address>(pingTarget.addr);
    cerr << "Selected Ping target: " << pingTarget.getAddress() << " at Node: " << source.getAddress() << endl;
    shared_ptr<BaseMessage> pingMsg = make_shared<PingMessage>(MsgTypes::Types::PING,
                                                                source,
                                                                pingTarget.addr,
                                                                currProtocolPeriod,
                                                                piggybackMembershipListEntries,
                                                                piggybackFailListEntries);

    vector<char> encodedPing = pingMsg->encode();
    int sizeSent = getEmulNet()->ENsend(&source, &(pingTarget.addr), encodedPing.data(), encodedPing.size());
    // Insert this id into pingMap so we can latter check to see if we successfully receive ack.
    // In the case that the msg is not successfully sent, we sould still update the map.
    // e.g.: Given the case that all nodes will drop all messages, each node should mark all the other nodes as failed.
    this->pingMap.insert(pingMsg->getId(), pingMsg->getSrc());
    if(sizeSent == 0) {
        cerr << "Ping msg from: " << source.getAddress()
                << " to: " << pingTarget.getAddress() << " is not sent." << endl;
        cerr << "Note that in this case, the pingMap will still be updated!" << endl;
        return false;
    } else {
        cerr << "Ping sent with id: " << pingMsg->getId() << endl;
        return true;
    }
}

bool MP1Node::sendPingReqMsg() {
    if(!this->currPingTarget) {
        cerr << "No pingTarget was selected, will not send pingReqMsg." << endl;
        return true;
    }

    Address source = getMemberNode()->addr;

    vector<MembershipListEntry> piggybackMembershipListEntries = getMembershipList().getTopK(K, getMaxPiggybackCnt());
    vector<FailListEntry> piggybackFailListEntries = getFailList().getTopK(K, getMaxPiggybackCnt());

    unsigned long currProtocolPeriod = getProtocolPeriod();

    cerr << "currProtocolPeriod: " << currProtocolPeriod << endl;

    // Select K targets to send PingReq message.
    vector<MembershipListEntry> pingReqTargets = getMembershipList().getRandomK(NUM_PING_REQ_TARGETS, source, *(this->currPingTarget));

    cerr << "PingReq targets selected at node: " << source.getAddress() << endl;
    for(const auto& pingReqTarget : pingReqTargets) {
        cerr << "\t" << pingReqTarget.getAddress() << endl;
    }

    for(auto& pingReqTarget : pingReqTargets) {
        // Build PingReqMsg.
        shared_ptr<BaseMessage> pingReqMsg = make_shared<PingReqMessage>(MsgTypes::Types::PING_REQ,
                                                                            source,
                                                                            pingReqTarget.addr,
                                                                            *(this->currPingTarget),
                                                                            this->protocolPeriodCnt,
                                                                            piggybackMembershipListEntries,
                                                                            piggybackFailListEntries);
        vector<char> encodedMsg = pingReqMsg->encode();
        int sizeSent = getEmulNet()->ENsend(&source, &(pingReqTarget.addr), encodedMsg.data(), encodedMsg.size());
        // Insert this id into pingReqMap so we can latter check to see if we successfully receive ack.
        this->pingReqMap.insert(pingReqMsg->getId(), pingReqMsg->getSrc());
        if(sizeSent == 0) {
            cerr << "PingReq msg from: " << source.getAddress()
                << " to: " << pingReqTarget.addr.getAddress() << " is not sent." << endl;
            cerr << "Note that in this case, the pingReqMap will still be updated!" << endl;
        } else {
            cout << "sizeSent is " << sizeSent << ", id: " << pingReqMsg->getId() << " will be inserted into pingReqMap." << endl;
        }
    }
    return true;
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

bool MP1Node::processPiggybackFailList(const vector<FailListEntry>& piggybackFailList) {
    // Need to remove all the entries from MembershipList if the entry is in piggybackFailList.
    // These removed entries will be inserted into this node's FailList, with piggybackCnt set to 0.
    for(const auto& failListEntry : piggybackFailList) {
        shared_ptr<MembershipListEntry> removedEntry = membershipList.removeEntry(failListEntry.addr.getAddress());
        if(removedEntry) {
            cerr << "Found piggyback failList entry: " << failListEntry << " in membership list: " << *removedEntry << endl;
            log->logNodeRemove(&(this->getMemberNode()->addr), &(removedEntry->addr));
            // Insert this entry to current node's failList.
            failList.insertEntry(*removedEntry);
        }
    }
    return true;
}

bool MP1Node::processPiggybackMembershipList(vector<MembershipListEntry> piggybackMembershipList) {
    // Remove entry from piggybackMembershipList that is alreay in current node's failList.
    for(auto it = piggybackMembershipList.begin(); it != piggybackMembershipList.end();) {
        if(failList.contains(it->getAddress())) {
            cerr << "piggybackMembershipList Entry: [" << *it << "] "
                << "is alreay in node: " << getMemberNode()->addr.getAddress() << "'s failList." << endl;
            it = piggybackMembershipList.erase(it);
        } else {
            it ++;
        }
    }
    for(const auto& entry : piggybackMembershipList) {
        auto localEntryPtr = membershipList.containsNode(entry.addr); 
        // If entry is not in current node's membershipList, just insert it.
        if(localEntryPtr == nullptr) {
            membershipList.insertEntryAtRandom(entry);
            log->logNodeAdd(&(this->getMemberNode()->addr), const_cast<Address*>(&(entry.addr)));
        } else {
            // If the received entry is of type ALIVE.
            if(entry.type == MemberTypes::ALIVE) {
                // If the entry is the node itself, we don't have to do anything.
                if(entry.addr.getAddress() == this->getMemberNode()->addr.getAddress()) {
                    continue;
                }
                // If local entry is also ALIVE.
                if(localEntryPtr->type == MemberTypes::ALIVE) {
                    // Update local incarnation if we receive a higher incarnation.
                    // And clear the piggybackCnt at the same time.
                    // {Alive Ml, inc = i} overrides {Alive Ml, inc = j} given [i > j].
                    if(entry.incarnationNum > localEntryPtr->incarnationNum) {
                        localEntryPtr->incarnationNum = entry.incarnationNum;
                        localEntryPtr->piggybackCnt = 0;
                    }
                } else {
                    // If local entry is currently marked SUSPECT.
                    // {Alive Ml, inc = i} overrides {Suspect Ml, inc = j} given [i > j].
                    if(entry.incarnationNum > localEntryPtr->incarnationNum) {
                        localEntryPtr->type = MemberTypes::ALIVE;
                        localEntryPtr->incarnationNum = entry.incarnationNum;
                        localEntryPtr->piggybackCnt = 0;
                    }
                }
            } else {
                // If the received entry is of type SUSPECT.
                // First, we check if the node itself is being suspected.
                if(entry.addr.getAddress() == this->getMemberNode()->addr.getAddress()) {
                    // Increase incarnation number at this node.
                    this->incarnationNum ++;
                    localEntryPtr->incarnationNum = this->incarnationNum;
                    localEntryPtr->piggybackCnt = 0;
                    continue;
                }
                // {Suspect Ml, inc = i} overrides {Alive Ml, inc = j} given [i >= j].
                if(localEntryPtr->type == MemberTypes::ALIVE) {
                    if(entry.incarnationNum >= localEntryPtr->incarnationNum) {
                        localEntryPtr->type = MemberTypes::SUSPECT;
                        localEntryPtr->incarnationNum = entry.incarnationNum;
                        localEntryPtr->piggybackCnt = 0;
                    }
                } else {
                    // {Suspect Ml, inc = i} overrides {Alive Ml, inc = j} given [i > j].
                    if(entry.incarnationNum > localEntryPtr->incarnationNum) {
                        localEntryPtr->incarnationNum = entry.incarnationNum;
                        localEntryPtr->piggybackCnt = 0;
                    }
                }
            }
        }
    }
    return true;
}

bool MP1Node::processPiggybackLists(const vector<MembershipListEntry>& piggybackMemershipList,
                                    const vector<FailListEntry>& piggybackFailList) {
    // We need to make sure piggybackFailList is being processed first.
    bool failSuccess = this->processPiggybackFailList(piggybackFailList);
    bool memberSuccess = this->processPiggybackMembershipList(piggybackMemershipList);
    return failSuccess && memberSuccess;
}

void MP1Node::dumpMembershipAndFailLists() {
    cerr << "\t\t************** " << getMemberNode()->addr.getAddress() <<" **************" << endl;
    cerr << "\t\t* MembershipList:" << endl;
    membershipList.printVec("\t\t* ");
    cerr << "\t\t* FailList:" << endl;
    failList.printVec("\t\t* ");
    cerr << "\t\t************** *** **************" << endl;
}

// The entries that are being suspected can stay for 4 protocol periods before getting moved to failList.
unsigned long MembershipListEntry::MAX_SUSPECT_TIMEOUT = 4 * MP1Node::PROTOCOL_PERIOD;