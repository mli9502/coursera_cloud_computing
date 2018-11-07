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

enum FailTypes {
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

class JoinEntry : Entry {
public:
	JoinEntry(Address addr) : Entry(addr) {}
	~JoinEntry() = default;
};

class FailEntry : Entry {
public:
	FailTypes type;
	long incarnationNum;

	FailEntry(Address addr, FailTypes type, long incarnationNum) : Entry(addr), type(type), incarnationNum(incarnationNum) {}	
	~FailEntry() = default;

	// TODO: Need to provide an update function to override the FailEntry with a new FailEntry. 
	// When a FailEntry is override, its piggybackCnt is set back to 0.
};

// TODO: Need to implement a class template, EntryList, that support lookup by address and getTopK().
// TODO: Need to have JoinEntryList and FailEntryList inherit EntryList<JoinEntry> and EntryList<FailEntry>.

template <typename T>
class EntryList {
public:
	// key: address, val: index to entryVec.
	unordered_map<string, typename vector<T>::iterator> entryMap;
	vector<T> entryVec;
	
	EntryList() : entryMap(), entryVec() {}
	~EntryList() {}

	bool insertEntry(T entry) {
		if(entryMap.find(entry.getAddress()) == entryMap.end()) {
			cerr << "Found duplicate entry in list: " << entry.getAddress() << endl;
			return false;
		}
		// Insert the entry at the start of vec, record this address in map.
		entryVec.insert(entryVec.begin(), entry);
		entryMap[entry.getAddress()] = entryVec.begin();
		// Get the order correct.
		this->reorderList();
		return true;
	}

	bool removeEntry(const string& address) {
		if(entryMap.find(address) == entryMap.end()) {
			cerr << "Address " << address << " is not in list..." << endl;
			return false; 
		}
		entryVec.erase(entryMap[address]);
		entryMap.erase(address);
		this->reorderList();
		return true;
	}
	/**
	 * Get top K entries with smallest piggyback cnt.
	 * Note that the return may be smaller than K since entryVec may have fewer elements than K.
	 * The piggyback cnt for these entries are increased at the same time.
	 */
	vector<T> getTopK(int k) {
		vector<T> rtn;
		for(int i = 0; i < k && i < entryVec.size(); i ++) {
			rtn.push_back(entryVec[i]);
			entryVec[i].incPiggybackCnt();
		}
		this->reorderList();
		return rtn;
	}

	void evictOutdatedEntry(int maxPiggybackCnt) {
		for(auto it = entryVec.begin(); it != entryVec.end();) {
			if(it->reachMaxPiggybackCnt(maxPiggybackCnt)) {
				entryMap.erase(it->getAddress());
				cerr << "Evict " << it->getAddress() << " from entry list..." << endl;
				it = entryVec.erase(it);
			} else {
				it ++;
			}
		}
		this->reorderList();
	}

private:
	/**
	 * Update entryVec to have teh correct order.
	 */ 
	void reorderList() {
		// First sort all entries by piggyback cnt.
		sort(entryVec.begin(), entryVec.end());
		// Then, for the entries with the same piggyback cnt, we shuffle them.
		int start = 0, end = 0;
		while(end <= entryVec.size()) {
			if(end == entryVec.size()) {
				std::random_shuffle(entryVec.begin() + start, entryVec.end());
			} else if(entryVec[end].piggybackCnt == entryVec[start].piggybackCnt) {
				end ++;
			} else {
				std::random_shuffle(entryVec.begin() + start, entryVec.begin() + end);
				start = end;
				end ++;
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
	int incarnationNum;
	// list of members that have joined the list.
	// entry: address <-> piggy-back cnt.
	EntryList<JoinEntry> joinList;
	// list of members that recently failed.
	EntryList<FailEntry> failList;
	// list of members that are currently alive.
	// TODO: 
	// When we add a new entry to joinList, we also have to add this entry to memberNode->memberList.
	// Similiarly, when we update an entry in failList to ALIVE, we have to update it in memberNode->memberList.
	// We don't have to delete the entry from failList when we add it to aliveList, because when its piggyback-cnt reach the maximum, it will automatically be removed.
	// When we get a fail for a member, we remove it from memberNode->memberList.
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
