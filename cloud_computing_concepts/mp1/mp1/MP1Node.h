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

#include <unordered_map>
#include <list>
#include <random>

#define TEST

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
enum MsgTypes {
    JOINREQ,
    JOINREP,
	// ping msg.
    PING,
	// ping-req msg.
	PING_REQ,
	// ack msg.
	ACK
};

enum MemberTypes {
	ALIVE,
	SUSPECT,
	FAIL
};

class Entry {
protected:
	// Msg representation for this entry.
	char* entryMsg;
public:
	Address addr;
	int piggybackCnt;

	Entry(Address addr) : entryMsg(nullptr), addr(addr), piggybackCnt(0) {}
	virtual ~Entry() {
		if(entryMsg) {
			delete [] entryMsg;
		}
	}

	bool operator<(const Entry& rhs) const {
		return piggybackCnt < rhs.piggybackCnt;
	}

	string getAddress() {
		return addr.getAddress();
	}

	void incPiggybackCnt() {
		piggybackCnt ++;
	}

	bool reachMaxPiggybackCnt(int maxPiggybackCnt) {
		return piggybackCnt > maxPiggybackCnt;
	}
};

class MembershipListEntry : public Entry {
protected:
	string getMemberTypeStr() const {
		vector<string> memberTypeStr {"ALIVE", "SUSPECT", "FAIL"};
		return memberTypeStr[(size_t)type];
	}
public:
	MemberTypes type;
	// NOTE: This incarnationNum is not the same as the member in MP1Node. 
	// This is the incarnationNum that is received from other nodes.
	long incarnationNum;

	MembershipListEntry(Address addr) : Entry(addr), type(MemberTypes::ALIVE), incarnationNum(0) {}
	MembershipListEntry(Address addr, MemberTypes type, long incarnationNum) : Entry(addr), type(type), incarnationNum(incarnationNum) {}	
	~MembershipListEntry() = default;

	// Get the size of this entry in a message.
	static unsigned getEntrySize() {
		// Address|MemberTypes|incarnationNum
		return sizeof(Address) + sizeof(MemberTypes) + sizeof(long);
	}

	// Convert this entry into char* message.
	char* getEntryMsg() {
		// First clear this msg.
		if(entryMsg != nullptr) {
			delete [] entryMsg;
		}
		entryMsg = new char[getEntrySize()];
		auto msgPtr = entryMsg;
		memcpy(msgPtr, &(this->addr), sizeof(Address));
		msgPtr += sizeof(Address);
		memcpy(msgPtr, &(this->type), sizeof(MemberTypes));
		msgPtr += sizeof(MemberTypes);
		memcpy(msgPtr, &(this->incarnationNum), sizeof(long));
		return entryMsg;
	}

	// Convert a msg back to entry.
	static MembershipListEntry decodeEntryMsg(char* msg) {
		Address addr;
		MemberTypes type;
		long incarnationNum = 0;
		memcpy(&addr, msg, sizeof(Address));
		msg += sizeof(Address);
		memcpy(&type, msg, sizeof(MemberTypes));
		msg += sizeof(MemberTypes);
		memcpy(&incarnationNum, msg, sizeof(long));
		return MembershipListEntry(addr, type, incarnationNum);
	}

	friend ostream& operator<<(ostream& os, const MembershipListEntry& rhs);
};

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
	char* getEntryMsg() {
		// First clear this msg.
		if(entryMsg != nullptr) {
			delete [] entryMsg;
		}
		entryMsg = new char[getEntrySize()];
		auto msgPtr = entryMsg;
		memcpy(msgPtr, &(this->addr), sizeof(Address));
		msgPtr += sizeof(Address);
		memcpy(msgPtr, &(this->type), sizeof(MemberTypes));
		return entryMsg;
	}

	// Convert a msg back to entry.
	static FailListEntry decodeEntryMsg(char* msg) {
		Address addr;
		MemberTypes type;
		long incarnationNum = 0;
		memcpy(&addr, msg, sizeof(Address));
		msg += sizeof(Address);
		memcpy(&type, msg, sizeof(MemberTypes));
		return FailListEntry(addr);
	}

	friend ostream& operator<<(ostream& os, const FailListEntry& rhs);
};

template <typename T>
class EntryList {
protected:
	char* topKMsg;
public:
	static std::mt19937::result_type SEED;

	// If we use vector, when we insert element in the vector, all the iterators after the insertion point will be invalidated!
	vector<T> entryVec;
	
	EntryList() : topKMsg(nullptr), entryVec() {}
	~EntryList() {
		if(topKMsg) {
			delete [] topKMsg;
		}
	}

	int getSize() {
		return entryVec.size();
	}

	bool removeEntry(const string& address) {
		for(auto it = entryVec.begin(); it != entryVec.end(); it ++) {
			if(it->getAddress() == address) {
				entryVec.erase(it);
				return true;
			}
		}
		cerr << "Address " << address << " is not in list..." << endl;
		return false; 
	}
	// number_of_entries|entry|entry|...
	// TODO: Need to update this method to take in the list of entries we want to encode.
	// 		 Basically, we want to seperate this with getTopK().
	//		 We want to getTopK first, then, pass the rtn from getTopK to genTopKMsg.
	//		 So, the BaseMessage::encode() do not have to take in any argument.
// 	pair<unsigned, char*> genTopKMsg(int k, int maxPiggybackCnt) {
// 		if(this->topKMsg) {
// 			delete [] this->topKMsg;
// 		}
// 		vector<T> topEntries = this->getTopK(k, maxPiggybackCnt);
// #ifdef TEST
// 		cout << "----------------------------------------" << endl;
// 		for(auto& entry : topEntries) {
// 			cout << entry << endl;
// 		}
// 		cout << "----------------------------------------" << endl;
// #endif
// 		vector<pair<unsigned, char*>> topMsgs;
// 		for(auto& entry : topEntries) {
// 			topMsgs.push_back(make_pair(entry.getEntrySize(), entry.getEntryMsg()));
// 		}
// 		unsigned numMsgs = topMsgs.size();
// 		// get total msg size.
// 		unsigned msgSize = sizeof(unsigned);
// 		for(auto& entry : topMsgs) {
// 			msgSize += entry.first;
// 		}
// 		this->topKMsg = new char[msgSize];
// 		auto tmpPtr = topKMsg;
// 		memcpy(tmpPtr, &numMsgs, sizeof(unsigned));
// 		tmpPtr += sizeof(unsigned);
// 		for(auto& entry : topMsgs) {
// 			memcpy(tmpPtr, entry.second, entry.first);
// 			tmpPtr += entry.first;
// 		}
// 		return make_pair(msgSize, this->topKMsg);
// 	}

	static string encodeTopKMsg(const vector<T>& topEntries) {
#ifdef TEST
		cout << "----------------------------------------" << endl;
		for(auto& entry : topEntries) {
			cout << entry << endl;
		}
		cout << "----------------------------------------" << endl;
#endif
		vector<pair<unsigned, char*>> topMsgs;
		for(auto& entry : topEntries) {
			topMsgs.push_back(make_pair(entry.getEntrySize(), entry.getEntryMsg()));
		}
		unsigned numMsgs = topMsgs.size();
		// get total msg size.
		unsigned msgSize = sizeof(unsigned);
		for(auto& entry : topMsgs) {
			msgSize += entry.first;
		}

		char* tmpPtr = new char[msgSize];
		memcpy(tmpPtr, &numMsgs, sizeof(unsigned));
		tmpPtr += sizeof(unsigned);
		for(auto& entry : topMsgs) {
			memcpy(tmpPtr, entry.second, entry.first);
			tmpPtr += entry.first;
		}
#ifdef TEST
		cout << "msgSize: " << msgSize << endl;
		cout << "msg: " << string(tmpPtr, msgSize) << endl;
#endif
		return string(tmpPtr, msgSize);
	}

	static void decodeTopKMsg(char*& msg, vector<T>& rtn) {
		unsigned numEntries = 0;
		memcpy(&numEntries, msg, sizeof(unsigned));
#ifdef TEST
		cout << "numEntries: " << numEntries << endl;
#endif
		msg += sizeof(unsigned);
		unsigned entrySize = T::getEntrySize();
#ifdef TEST
		cout << "entrySize: " << entrySize << endl;
#endif
		for(unsigned i = 0; i < numEntries; i ++) {
			rtn.push_back(T::decodeEntryMsg(msg));
			msg += entrySize;
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
	vector<T> getTopK(int k, int maxPiggybackCnt) {
		vector<T> rtn;
		vector<pair<T, int>> tmpVec;
		for(int i = 0; i < entryVec.size(); i ++) {
			tmpVec.push_back({entryVec[i], i});
		}
		this->reorderVec(tmpVec);
#ifdef TEST
		cout << "+++++++++++++++++++++++++++++" << endl;
		for(auto& entry : tmpVec) {
			cout << entry.first << endl;
		}
		cout << "+++++++++++++++++++++++++++++" << endl;
#endif
		auto it = tmpVec.begin();
		while(rtn.size() < k && it != tmpVec.end()) {
			if(it->first.reachMaxPiggybackCnt(maxPiggybackCnt)) {
				break;
			}
			rtn.push_back(it->first);
			entryVec[it->second].incPiggybackCnt();
			it ++;
		}
		return rtn;
	}
	
	void printVec() {
		for(auto& entry : this->entryVec) {
			cout << entry << endl;
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
	int lastPingIdx;
	MembershipList() : EntryList(), lastPingIdx(0) {}
	~MembershipList() = default;

	// Insert membership list entry we received.
	// If the address of the entry is already present in the list, we update the entry using override rules.
	// If the address is new, this means that a new entry has joined. And we insert this new entry into a random location in the list.
	bool insertEntry(MembershipListEntry newEntry) {
		cerr << "inserting: " << newEntry << endl;
		for(auto& entry : this->entryVec) {
			if(newEntry.getAddress() == entry.getAddress()) {
				cerr << "Entry is already presented in membership list: " << endl;
				cerr << "Entry in list: " << entry << endl;
				cerr << "Received entry: " << newEntry << endl;
				if(isOverride(newEntry, entry)) {
					entry.type = newEntry.type;
					entry.incarnationNum = newEntry.incarnationNum;
					return true;
				}
				return false;
			}
		}
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

	// Get the ping target and advance lastPingEntryIdx.
	bool getPingTarget(MembershipListEntry& rtn, const Address& selfAddr) {
		if(entryVec.empty()) {
			cerr << "Membership list is currently empty!" << endl;
			return false;
		}
		if(lastPingIdx == entryVec.size()) {
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

private:
	bool isOverride(const MembershipListEntry& newEntry, const MembershipListEntry& oldEntry) {
		auto newType = newEntry.type, oldType = oldEntry.type;
		auto newIncarnation = newEntry.incarnationNum, oldIncarnation = oldEntry.incarnationNum;
		if(newType == MemberTypes::ALIVE) {
			return (oldType == MemberTypes::SUSPECT && newIncarnation > oldIncarnation) ||
					(oldType == MemberTypes::ALIVE && newIncarnation > oldIncarnation);
		} else if(newType == MemberTypes::SUSPECT) {
			return (oldType == MemberTypes::SUSPECT && newIncarnation > oldIncarnation) ||
					(oldType == MemberTypes::ALIVE && newIncarnation >= oldIncarnation);
		} else if(newType == MemberTypes::FAIL) {
			return oldType == MemberTypes::ALIVE || oldType == MemberTypes::SUSPECT;
		}
		return false;
	}
};

// Fail list
class FailList : public EntryList<FailListEntry> {
public:
	FailList() : EntryList() {}
	~FailList() = default;

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
	enum MsgTypes msgType;
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

	// Incarnation number.
	// Initialized to 0 when node joined. Incremented when it receives a SUSPECT of itself.
	// When a SUSPECT msg on the node itself comes, insert this node into membership list with piggybackCnt 0, type ALIVE and the updated incarnation number.
	// By doing this, the ALIVE information is send to other nodes. 
	long incarnationNum;
	// list of members that have been failed recently.
	FailList failList;
	// list of members that recently failed.
	MembershipList membershipList;
	// Count for protocol period.
	// This count is needed to form the ID for a message.
	// The ID is IP:PORT:PERIOD_CNT
	unsigned long protocolPeriodCnt;
	// pingMap, pingReqMap and pingReqPingMap stores the outstanding msgs that we sent out.
	// key: ID, val: msg we sent
	// ID = src_ip|dest_ip|protocol_period_cnt
	// pingMap: The initial ping message we send every protocol period. 
	TimeoutMap<string, string> pingMap;
	// pingReqMap: The ping_req msgs we send out after the initial ping request times out.
	TimeoutMap<string, string> pingReqMap;
	// pingReqPingMap: The ping msgs we send out after we receives a ping_req msg. 
	TimeoutMap<string, string> pingReqPingMap;
	// list of members that are currently alive.
	int getMaxPiggybackCnt();

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
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
	virtual ~MP1Node();

	pair<unsigned, char*> genPingMsg(MembershipListEntry to, Address idAddr, unsigned long idPeriodCnt);
	pair<unsigned, char*> genPingReqMsg(MembershipListEntry to, MembershipListEntry req, Address idAddr, unsigned long idPeriodCnt);
	pair<unsigned, char*> genAckMsg(MembershipListEntry to, Address idAddr, unsigned long idPeriodCnt);

	MsgTypes getMsgType(char* msg) {
		MsgTypes rtn;
		memcpy(&rtn, msg, sizeof(MsgTypes));
		return rtn;
	}

	string getMsgId(char* msg) {
		msg += sizeof(MsgTypes);
		Address idAddr;
		unsigned long idPeriodCnt;
		memcpy(&idAddr, msg, sizeof(Address));
		msg += sizeof(Address);
		memcpy(&idPeriodCnt, msg, sizeof(unsigned long));
		return getId(idAddr, idPeriodCnt);
	}

	void decodePingMsg(char* msg, 
						Address& idAddr, unsigned long& idPeriodCnt,
						Address& fromAddr, Address& toAddr,
						vector<MembershipListEntry>& membershipListTopK,
						vector<FailListEntry>& failListTopK);

	static string getId(Address idAddr, unsigned long idPeriodCnt) {
		return idAddr.getAddress() + ":" + to_string(idPeriodCnt);
	}

	template <typename T>
	static void copyMsg(char*& msg, T field) {
		memcpy(msg, &field, sizeof(T));
		msg += sizeof(T);
	}

	template <typename T>
	static void copyObj(char*& msg, T& field) {
		memcpy(&field, msg, sizeof(T));
		msg += sizeof(T);
	}
};

class MsgHelper {
public:
	static const vector<string> msgTypeStrs;
	static MsgTypes getMsgType(char* data);
	static string getMsgTypeStr(MsgTypes mt);
};

#endif /* _MP1NODE_H_ */
