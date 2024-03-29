/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

#include "TimeoutMap.h"
#include "util.h"

#include <unordered_map>
#include <list>
#include <random>
#include <memory>

// #define TEST

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define SEED_CONST 0

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */

struct MsgTypes {
	enum Types {
		JOINREQ = 0,
		JOINRESP = 1,
		// ping msg.
		PING = 2,
		// ping-req msg.
		PING_REQ = 3,
		// ack msg.
		ACK = 4
	};

	static std::string to_string(const MsgTypes::Types type) {
		switch(type) {
			case JOINREQ:
				return "JOINREQ";
				break;
			case JOINRESP:
				return "JOINRESP";
				break;
			case PING:
				return "PING";
				break;
			case PING_REQ:
				return "PING_REQ";
				break;
			case ACK:
				return "ACK";
				break;
			default:
				return "INVALID_MSG_TYPE";
		}
	}

};


enum MemberTypes {
	ALIVE,
	SUSPECT,
	FAIL
};

class Entry {
public:
	Address addr;
	int piggybackCnt;

	Entry() : piggybackCnt(0) {}
	Entry(Address addr) : addr(addr), piggybackCnt(0) {}
	Entry(const Entry& entry) : addr(entry.addr), piggybackCnt(entry.piggybackCnt) {}

	virtual ~Entry() = default;

	bool operator<(const Entry& rhs) const {
		return piggybackCnt < rhs.piggybackCnt;
	}

	string getAddress() const {
		return addr.getAddress();
	}

	void incPiggybackCnt() {
		piggybackCnt ++;
	}

	bool reachMaxPiggybackCnt(int maxPiggybackCnt) {
		return piggybackCnt > maxPiggybackCnt;
	}

	void clearPiggybackCnt() {
		piggybackCnt = 0;
	}
};

class MembershipListEntry : public Entry {
protected:
	string getMemberTypeStr() const {
		vector<string> memberTypeStr {"ALIVE", "SUSPECT", "FAIL"};
		return memberTypeStr[(size_t)type];
	}
public:
	static unsigned long MAX_SUSPECT_TIMEOUT;

	MemberTypes type;
	// NOTE: This incarnationNum is not the same as the member in MP1Node. 
	// This is the incarnationNum that is received from other nodes.
	unsigned long incarnationNum;
	unsigned long suspectTimeout;

	MembershipListEntry() : Entry(), type(MemberTypes::ALIVE), incarnationNum(0), suspectTimeout(0) {}
	MembershipListEntry(Address addr) : Entry(addr), type(MemberTypes::ALIVE), incarnationNum(0), suspectTimeout(0) {}
	MembershipListEntry(Address addr, MemberTypes type, unsigned long incarnationNum) : Entry(addr), type(type), incarnationNum(incarnationNum), suspectTimeout(0) {}	
	MembershipListEntry(const MembershipListEntry& entry) : Entry(entry), type(entry.type), incarnationNum(entry.incarnationNum), suspectTimeout(0) {}

	~MembershipListEntry() = default;

	// Get the size of this entry in a message.
	static unsigned getEntrySize() {
		// Address|MemberTypes|incarnationNum
		return sizeof(Address) + sizeof(MemberTypes) + sizeof(long);
	}

	// Convert this entry into char* message.
	vector<char> getEntryMsg() const {
		vector<char> entryMsg(getEntrySize());
		char* msgPtr = &entryMsg[0];
		memcpy(msgPtr, &(this->addr), sizeof(Address));
		msgPtr += sizeof(Address);
		memcpy(msgPtr, &(this->type), sizeof(MemberTypes));
		msgPtr += sizeof(MemberTypes);
		memcpy(msgPtr, &(this->incarnationNum), sizeof(long));
		return entryMsg;
	}

	void incSuspectTimeout(bool& reachMaxSuspectTime) {
		suspectTimeout ++;
		reachMaxSuspectTime = (suspectTimeout == MAX_SUSPECT_TIMEOUT);
	}

	// Convert a msg back to entry.
	static MembershipListEntry decodeEntryMsg(char const*& msgPtr) {
		Address addr;
		MemberTypes type;
		unsigned long incarnationNum = 0;
		copyObj(msgPtr, addr);
		copyObj(msgPtr, type);
		copyObj(msgPtr, incarnationNum);
		return MembershipListEntry(addr, type, incarnationNum);
	}

	MemberTypes getType() {
		return type;
	}

	void markAsSuspected() {
		this->piggybackCnt = 0;
		this->type = SUSPECT;
	}
	
	void markAsAlive() {
		this->piggybackCnt = 0;
		this->type = ALIVE;
	}

	friend ostream& operator<<(ostream& os, const MembershipListEntry& rhs);
};
// TODO: What should we put in FailListEntry?
// Only an Address should be enough. What is evictTimeout needed for?
// evictTimeout is needed for deleting the entries that has been in list for too long.
class FailListEntry : public Entry {
public:
	static long MAX_LIVE_TIME;

	MemberTypes type;
	long evictTimeout;	

	FailListEntry(Address addr) : Entry(addr), type(MemberTypes::FAIL), evictTimeout(0) {}
	FailListEntry(const Entry& entry) : Entry(entry.addr), type(MemberTypes::FAIL), evictTimeout(0) {}
	~FailListEntry() = default;

	// Get the size of this entry in a message.
	static unsigned getEntrySize() {
		// Address|MemberTypes
		// Note that although we include the type in the message, the type will always be FAIL.
		return sizeof(Address) + sizeof(MemberTypes);
	}

	// Convert this entry into char* message.
	vector<char> getEntryMsg() const {
		vector<char> entryMsg(getEntrySize());
		char* msgPtr = &entryMsg[0];
		memcpy(msgPtr, &(this->addr), sizeof(Address));
		msgPtr += sizeof(Address);
		memcpy(msgPtr, &(this->type), sizeof(MemberTypes));
		return entryMsg;
	}

	// Convert a msg back to entry.
	static FailListEntry decodeEntryMsg(char const*& msgPtr) {
		Address addr;
		MemberTypes type;
		copyObj(msgPtr, addr);
		copyObj(msgPtr, type);
		return FailListEntry(addr);
	}

	friend ostream& operator<<(ostream& os, const FailListEntry& rhs);
};

template <typename T>
class EntryList {
public:
	static std::mt19937::result_type SEED;

	// If we use vector, when we insert element in the vector, all the iterators after the insertion point will be invalidated!
	vector<T> entryVec;
	
	EntryList() : entryVec() {}
	~EntryList() = default;

	int getSize() {
		return entryVec.size();
	}

	bool contains(const Address& address) {
		return contains(address.getAddress());
	}

	bool contains(const string& address) {
		for(const auto& entry : entryVec) {
			if(entry.getAddress() == address) {
				return true;
			}
		}
		return false;
	}

	bool removeEntry(const Address& address) {
		return removeEntry(address.getAddress());
	}

	shared_ptr<T> removeEntry(const string& address) {
		for(auto it = entryVec.begin(); it != entryVec.end(); it ++) {
			if(it->getAddress() == address) {
				shared_ptr<T> rtn = make_shared<T>(*it);
				entryVec.erase(it);
				return rtn;
			}
		}
		cerr << "Address " << address << " is not in list..." << endl;
		return nullptr; 
	}

	static vector<char> encodeTopKMsg(const vector<T>& topEntries) {
		vector<char> msg;
		// Insert number of entries at the start of message.
		vector<char> entryCntMsg(sizeof(unsigned));
		unsigned numEntries = topEntries.size();
		memcpy(&entryCntMsg[0], &numEntries, sizeof(unsigned));
		msg.insert(msg.end(), entryCntMsg.begin(), entryCntMsg.end());
		// Insert entries into message.
		for(auto& entry : topEntries) {
			const auto& entryMsg = entry.getEntryMsg();
			msg.insert(msg.end(), entryMsg.begin(), entryMsg.end());
		}

		return msg;
	}

	static void decodeTopKMsg(char const*& msgPtr, vector<T>& rtn) {
		unsigned numEntries = 0;
		memcpy(&numEntries, msgPtr, sizeof(unsigned));
		msgPtr += sizeof(unsigned);
		for(unsigned i = 0; i < numEntries; i ++) {
			rtn.push_back(T::decodeEntryMsg(msgPtr));
		}
	}

	/**
	 * Get top K entries with smallest piggyback cnt.
	 * Note that the return may be smaller than K since entryList may have fewer elements than K.
	 * The piggyback cnt for these entries are increased at the same time.
	 * 
	 * maxPiggybackCnt is a number that we define in parameter.
	 * 
	 * NOTE: we do not remove entries that exceeds piggyback-cnt threshold from the membership list, we simply ignore them.
	 */
	vector<T> getTopK(int K, int maxPiggybackCnt) {
		vector<T> rtn;
		vector<pair<T, int>> tmpVec;
		for(int i = 0; i < entryVec.size(); i ++) {
			tmpVec.push_back({entryVec[i], i});
		}
		this->reorderVec(tmpVec);
		auto it = tmpVec.begin();
		while(rtn.size() < K && it != tmpVec.end()) {
			if(it->first.reachMaxPiggybackCnt(maxPiggybackCnt)) {
				break;
			}
			rtn.push_back(it->first);
			entryVec[it->second].incPiggybackCnt();
			it ++;
		}
		return rtn;
	}

	/**
	 * Get the first K entries. This is used by JoinResp message to send membership & fail list entries.
	 * We get the first instead of get random K to make sure the result is consistant.
	 */
	vector<T> getFirstK(int K) {
		vector<T> rtn;
		for(unsigned i = 0; i < entryVec.size() && i < K; i ++) {
			rtn.push_back(entryVec.at(i));
		}
		return rtn;
	}
	
	void printVec(const string& prefix = "") {
		for(auto& entry : this->entryVec) {
			cout << prefix << entry << endl;
		}
	}
	/**
	 * Update entryList to have the correct order based on piggyback cnt.
	 */ 
	template <typename U>
	void reorderVec(vector<U>& vec) {
		// First sort all entries by piggyback cnt.
		sort(vec.begin(), vec.end());
		// Then, for the entries with the same piggyback cnt, we shuffle them.
		auto startIt = vec.begin(), endIt = vec.begin();
		while(true) {
			if(endIt == vec.end()) {
				if(SEED != 0) {
					std::shuffle(startIt, endIt, std::mt19937{SEED});
				} else {
					std::shuffle(startIt, endIt, std::mt19937{std::random_device{}()});
				}
				break;
			} else if(endIt->first.piggybackCnt == startIt->first.piggybackCnt) {
				endIt ++;
			} else {
				if(SEED != 0) {
					std::shuffle(startIt, endIt, std::mt19937{SEED});
				} else {
					std::shuffle(startIt, endIt, std::mt19937{std::random_device{}()});
				}
				startIt = endIt;
				endIt ++;
			}
		}
	}
};

template<typename T>
std::mt19937::result_type EntryList<T>::SEED = SEED_CONST;

// Membership list.
class MembershipList : public EntryList<MembershipListEntry> {
public:
	// Index for the last ping target we selected.
	unsigned lastPingIdx;
	MembershipList() : EntryList(), lastPingIdx(0) {}
	~MembershipList() = default;

	// Check if a node is already in the list.
	MembershipListEntry* containsNode(const Address& addr) {
		for(auto& entry : this->entryVec) {
			if(entry.getAddress() == addr.getAddress()) {
				return &entry;
			}
		}
		return nullptr;
	}

	// Note that appendEntry will not check for duplicate.
	bool appendEntry(const Address& addr) {
		return appendEntry(MembershipListEntry(addr));
	}

	bool appendEntry(MembershipListEntry newEntry) {
		entryVec.push_back(newEntry);
		return true;
	}

	bool insertEntryAtRandom(const Address& addr) {
		return insertEntryAtRandom(MembershipListEntry(addr));
	}

	// TODO: @7/2/2019: May need to deprecate this method, since when we get top K, and and ping target, we will do random again,
	// 					there's no point of random at insert time.
	// Insert membership list entry we received at a random position in the list.
	// If the address of the entry is already present in the list, we update the entry using override rules.
	// If the address is new, this means that a new entry has joined. And we insert this new entry into a random location in the list.
	bool insertEntryAtRandom(MembershipListEntry newEntry) {
		int randIdx = 0;
		if(!entryVec.empty()) {
			int left = 0, right = entryVec.size() - 1;
			if(EntryList<MembershipListEntry>::SEED != 0) {
				srand(static_cast<unsigned>(EntryList<MembershipListEntry>::SEED));
			} else {
				srand(time(NULL));
			}
			randIdx = rand() % (right - left + 1) + left;
		}
		auto it = this->entryVec.begin();
		std::advance(it, randIdx);
		entryVec.insert(it, newEntry);
		return true;
	}

	/**
	 * Get random K entries from the list. 
	 * This is used to select targets to send PingReq message.
	 */
	vector<MembershipListEntry> getRandomK(unsigned K, const Address& selfAddr, const Address& pingTargetAddr) {
		vector<MembershipListEntry> rtn;
		vector<pair<MembershipListEntry, int>> tmpVec;
		for(unsigned i = 0; i < entryVec.size(); i ++) {
			tmpVec.push_back({entryVec[i], i});
		}
		this->reorderVec(tmpVec);
		auto it = tmpVec.begin();
		while(rtn.size() < K && it != tmpVec.end()) {
			// Skip the pingTarget that we already send PingMsg to.
			if(it->first.addr == pingTargetAddr || it->first.addr == selfAddr) {
				it ++;
				continue;
			}
			rtn.push_back(it->first);
			it ++;
		}
		return rtn;
	}

	// Get the ping target and advance lastPingEntryIdx.
	bool getPingTarget(MembershipListEntry& rtn, const Address& selfAddr) {
		if(entryVec.empty()) {
			cerr << "Membership list is currently empty!" << endl;
			return false;
		}
		if(entryVec.size() == 1 && entryVec[0].addr == selfAddr) {
			cerr << "Membership list only contain selfAddr!" << endl;
			return false;
		}
		// Note that in the current implementation, the very first Ping iteration will not shuffle the list.
		if(lastPingIdx >= entryVec.size()) {
			if(EntryList<MembershipListEntry>::SEED != 0) {
				std::shuffle(entryVec.begin(), entryVec.end(), std::mt19937{EntryList<MembershipListEntry>::SEED});
			} else {
				std::shuffle(entryVec.begin(), entryVec.end(), std::mt19937{std::random_device{}()});
			}
			lastPingIdx = 0;
		}
		// If the next entry is the current node itself, ignore it.
		if(entryVec[lastPingIdx].addr == selfAddr) {
			lastPingIdx ++;
			return getPingTarget(rtn, selfAddr);
		}
		rtn = entryVec[lastPingIdx];
		lastPingIdx ++;
		return true;
	}
};

// Fail list
class FailList : public EntryList<FailListEntry> {
public:
	FailList() : EntryList() {}
	~FailList() = default;

	bool insertEntry(FailListEntry newEntry) {
		for(auto& entry : entryVec) {
			if(newEntry.getAddress() == entry.getAddress()) {
				cerr << "Entry is already marked as failed..." << endl;
				cerr << entry << endl;
				return false;
			}
		}
		entryVec.push_back(newEntry);
		entryVec.back().piggybackCnt = 0;
		return true;
	}

	bool insertEntry(MembershipListEntry newEntry) {
		for(auto& entry : entryVec) {
			if(newEntry.getAddress() == entry.getAddress()) {
				cerr << "Entry is already marked as failed..." << endl;
				cerr << entry << endl;
				return false;
			}
		}
		entryVec.push_back(FailListEntry(newEntry));
		entryVec.back().piggybackCnt = 0;

		return true;
	}

	/**
	 * Fail entry is only kept in the list for a given period.
	 * After this period, they are removed.
	 */
	void evictTimeoutEntries() {
		for(auto it = entryVec.begin(); it != entryVec.end(); ) {
			if(it->evictTimeout >= FailListEntry::MAX_LIVE_TIME) {
				it = entryVec.erase(it);
			} else {
				it ++;
			}
		}
	}
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes::Types msgType;
} MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

public:
	// Ping timeout.
	// PING_TIMEOUT = RTT = 2 for now.
	static const long PING_TIMEOUT;
	// Ping-req timeout.
	// PING_REQ_TIMEOUT = 2 * PING_TIMEOUT = 4 for now.
	static const long PING_REQ_TIMEOUT;
	// TODO: Protocal period.
	// PROTOCOL_PERIOD = PING_TIMEOUT + PING_REQ_TIMEOUT + 2 = 3 * RTT + 2 = 8.
	static const long PROTOCOL_PERIOD;

	// Every time, K entries from MembershipList, and FailList are selected for piggyback to the msg.
	static const int K;
	// Each entry is piggy-backed at most lambda * log(N) times. Where N = aliveList.size().
	static const int lambda;

	// Number of targets we select each time for sending PING_REQ message.
	static const int NUM_PING_REQ_TARGETS;
private:
	// Incarnation number.
	// Initialized to 0 when node joined. Incremented when it receives a SUSPECT of itself.
	// When a SUSPECT msg on the node itself comes, insert this node into membership list with piggybackCnt 0, type ALIVE and the updated incarnation number.
	// By doing this, the ALIVE information is send to other nodes. 
	unsigned long incarnationNum;
	// list of members that have been failed recently.
	FailList failList;
	// list of members that recently failed.
	MembershipList membershipList;
	// Count for protocol period indicating how many period we have passed by.
	// This count is needed to form the ID for a message.
	// The ID is IP:PORT:PERIOD_CNT
	unsigned long protocolPeriodCnt;
	// Counter indicating whether we have reached PROTOCOL_PERIOD.
	// Will be reset to 0 once reach PROTOCOL_PERIOD.
	// Will be incremented every recvLoop.
	unsigned protocolPeriodLocalCounter;
	shared_ptr<Address> currPingTarget;
	bool ackReceived;
	// pingMap, pingReqMap and pingReqPingMap stores the outstanding msgs that we sent out.
	// Every time we receive an Ack message, do the following:
	// - First check if sourceId is in pingMap. If yes, remove the entry from pingMap. mark ackReceived to true.
	// - Then, check if sourceId is in pingReqMap. If yes, remove the entry from pingMap, mark ackReceived to true.
	// - Then, check if sourceId is in pingReqPingMap. If yes, redirect the Ack msg by sending an Ack to the node that sends the PingReq.
	
	// ID: src_ip|dest_ip|protocol_period_cnt

	// key: ID
	// val: curr node's address. (val will not be used)
	// pingMap: The initial ping message we send every protocol period.
	TimeoutMap<string, Address> pingMap;
	// pingReqMap: The ping_req msgs we send out after the initial ping request times out.
	// key: ID
	// val: curr node's address. (val will not be used)
	TimeoutMap<string, Address> pingReqMap;
	// pingReqPingMap: The ping msgs we send out after we receives a ping_req msg. 
	// key: ID
	// val: the Address of the source address that we eventually need to send an ACK msg to.
	TimeoutMap<string, Address> pingReqPingMap;

	bool sendPingMsg();
	bool sendPingReqMsg();

	void handleSuccessPing();
	void handleFailedPing();

	// Process the received FailList.
	bool processPiggybackFailList(const vector<FailListEntry>& piggybackFailList);
	// Process the received MembershipList.
	bool processPiggybackMembershipList(vector<MembershipListEntry> piggybackMembershipList);

	void dumpMembershipAndFailLists();

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	Log* getLogHandle() {
		return this->log;
	}

	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);

	void updatePeriod() {
		protocolPeriodLocalCounter ++;
		if(protocolPeriodLocalCounter == PROTOCOL_PERIOD) {
			protocolPeriodLocalCounter = 0;
			protocolPeriodCnt ++;
		}
	}

	MembershipList& getMembershipList();
	FailList& getFailList();
	EmulNet* getEmulNet();
	
	virtual ~MP1Node();

	// list of members that are currently alive.
	int getMaxPiggybackCnt();
	unsigned long getProtocolPeriod() const {
		return this->protocolPeriodCnt;
	}
	unsigned long getIncarnationNum() const {
		return this->incarnationNum;
	}

	TimeoutMap<string, Address>& getPingMap() {
		return this->pingMap;
	}	
	TimeoutMap<string, Address>& getPingReqMap() {
		return this->pingReqMap;
	}
	TimeoutMap<string, Address>& getPingReqPingMap() {
		return this->pingReqPingMap;
	}

	void setAckReceived() {
		this->ackReceived = true;
	}
	// Process piggyback lists.
	bool processPiggybackLists(const vector<MembershipListEntry>& piggybackMembershipList, 
								const vector<FailListEntry>& piggybackFailList);
};

class MsgHelper {
public:
	static const vector<string> msgTypeStrs;
	static MsgTypes::Types getMsgType(char* data);
	static string getMsgTypeStr(MsgTypes::Types mt);
};

#endif /* _MP1NODE_H_ */
