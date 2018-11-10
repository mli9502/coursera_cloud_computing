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

#include <unordered_map>
#include <list>
#include <random>

// #define TEST

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes {
    JOINREQ,
    JOINREP,
    DUMMYLASTMSGTYPE
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

	Entry(Address addr) : addr(addr), piggybackCnt(0) {}
	virtual ~Entry() = default;

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
	// This is received from other nodes.
	long incarnationNum;

	MembershipListEntry(Address addr) : Entry(addr), type(MemberTypes::ALIVE), incarnationNum(0) {}
	MembershipListEntry(Address addr, MemberTypes type, long incarnationNum) : Entry(addr), type(type), incarnationNum(incarnationNum) {}	
	~MembershipListEntry() = default;

	friend ostream& operator<<(ostream& os, const MembershipListEntry& rhs);
};

template <typename T>
class EntryList {
public:
	// If we use vector, when we insert element in the vector, all the iterators after the insertion point will be invalidated!
	vector<T> entryVec;
	
	EntryList() : entryVec() {}
	~EntryList() {}

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
	/**
	 * Get top K entries with smallest piggyback cnt.
	 * Note that the return may be smaller than K since entryList may have fewer elements than K.
	 * The piggyback cnt for these entries are increased at the same time.
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

	static std::mt19937::result_type SEED;
	/**
	 * Update entryList to have teh correct order based on piggyback cnt.
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
std::mt19937::result_type EntryList<T>::SEED = 0;

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
					entry = newEntry;
					return true;
				}
				return false;
			}
		}
		int randIdx = 0;
		if(!entryVec.empty()) {
			int left = 0, right = entryVec.size() - 1;
			randIdx = rand() % (right - left + 1) + left;
		}
		auto it = this->entryVec.begin();
		std::advance(it, randIdx);
		entryVec.insert(it, newEntry);
		return true;
	}

	// Get the ping target and advance lastPingEntryIdx.
	bool getPingTarget(MembershipListEntry& rtn) {
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

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

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
	// Every time, K entries from joinList, and faileList are selected for piggyback to the msg.
	static const int K;
	// Each entry is piggy-backed at most lambda * log(N) times. Where N = aliveList.size().
	static const int lambda;
	// Incarnation number.
	// Initialized to 0 when node joined. Incremented when it receives a SUSPECT of itself.
	long incarnationNum;
	// lastPingAddress.
	Address* pLastPingAddress;
	// list of members that have joined the list.
	// entry: address <-> piggy-back cnt.
	// TODO:@11/9/2018: Need to add FailList class.
	EntryList<MembershipListEntry> failList;
	// list of members that recently failed.
	MembershipList membershipList;
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
};

class MsgHelper {
public:
	static const vector<string> msgTypeStrs;
	static MsgTypes getMsgType(char* data);
	static string getMsgTypeStr(MsgTypes mt);
};

#endif /* _MP1NODE_H_ */
