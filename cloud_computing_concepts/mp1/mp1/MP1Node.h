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

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

// helper function to shuffle the list...
template <typename T> 
void shuffle_list(typename list<T>::iterator begin, typename list<T>::iterator end) {
    // create a vector of (wrapped) references to elements in the list
    // http://en.cppreference.com/w/cpp/utility/functional/reference_wrapper
    std::vector<T> vec(begin, end);
    // shuffle (the references in) the vector
    std::shuffle(vec.begin(), vec.end(), std::mt19937{std::random_device{}()});
	std::copy(vec.begin(), vec.end(), begin);
}

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
	// key: address, val: index to entryList.
	unordered_map<string, typename list<T>::iterator> entryMap;
	// TODO:@11/9/2018: Need to change this to list!
	// If we use vector, when we insert element in the vector, all the iterators after the insertion point will be invalidated!
	list<T> entryList;
	
	EntryList() : entryMap(), entryList() {}
	~EntryList() {}

	int getSize() {
		return entryList.size();
	}

	bool removeEntry(const string& address) {
		if(entryMap.find(address) == entryMap.end()) {
			cerr << "Address " << address << " is not in list..." << endl;
			return false; 
		}
		entryList.erase(entryMap[address]);
		entryMap.erase(address);
		return true;
	}
	/**
	 * Get top K entries with smallest piggyback cnt.
	 * Note that the return may be smaller than K since entryList may have fewer elements than K.
	 * The piggyback cnt for these entries are increased at the same time.
	 * 
	 * NOTE: we do not remove entries that exceeds piggyback-cnt threshold from the membership list, we simply ignore them.
	 */
	vector<T> getTopK(int k, int maxPiggybackCnt) {
		// FIXME: We need to remove the map...
		// When we sort the list, we sort it using list<pair<T, typename list<T>::iterator>> so we can latter easily access those elements in origional list.
		vector<T> rtn;
		list<T> tmpList = entryList;
		cout << "after copy" << endl;
		this->reorderList(tmpList);
		cout << "after reorder" << endl;
		for(auto& entry : tmpList) {
			cout << entry << endl;
		}
		auto it = tmpList.begin();
		while(rtn.size() < k && it != tmpList.end()) {
			if(it->reachMaxPiggybackCnt(maxPiggybackCnt)) {
				break;
			}
			rtn.push_back(*it);
			it ++;
		}
		cout << "here..." << endl;
		for(auto& entry : rtn) {
			cout << entry.getAddress() << endl;
			// if(entryMap.find(entry.getAddress()) == entryMap.end()) {
			// 	cerr << "entry not found..." << endl;
			// }
			entryMap[entry.getAddress()]->incPiggybackCnt();
		}
		return rtn;
	}
	
	void printList() {
		for(auto& entry : this->entryList) {
			cout << entry << endl;
		}
	}

	void printMap() {
		for(auto& entry : this->entryMap) {
			cout << entry.first << ": " << *(entry.second) << endl;
		}
	}

private:
	/**
	 * Update entryList to have teh correct order based on piggyback cnt.
	 */ 
	void reorderList(list<T>& inputList) {
		// First sort all entries by piggyback cnt.
		sort(inputList.begin(), inputList.end());
		// Then, for the entries with the same piggyback cnt, we shuffle them.
		auto startIt = inputList.begin(), endIt = inputList.begin();
		while(true) {
			if(endIt == inputList.end()) {
				shuffle_list<T>(startIt, endIt);
				break;
			} else if(endIt->piggybackCnt == startIt->piggybackCnt) {
				endIt ++;
			} else {
				shuffle_list<T>(startIt, endIt);
				startIt = endIt;
				endIt ++;
			}
		}
	}
};

// Membership list.
class MembershipList : public EntryList<MembershipListEntry> {
public:
	// Index for the last ping target we selected.
	list<MembershipListEntry>::iterator lastPingIt;
	MembershipList() : EntryList(), lastPingIt(entryList.end()) {}
	~MembershipList() = default;

	// Insert membership list entry we received.
	// If the address of the entry is already present in the list, we update the entry using override rules.
	// If the address is new, this means that a new entry has joined. And we insert this new entry into a random location in the list.
	bool insertEntry(MembershipListEntry entry) {
		cout << "inserting: " << entry << endl;
		if(entryMap.find(entry.getAddress()) != entryMap.end()) {
			cerr << "Entry is already presented in membership list: " << endl;
			cerr << "Entry in list: " << *(entryMap[entry.getAddress()]) << endl;
			cerr << "Received entry: " << entry << endl;
			if(isOverride(entry, *(entryMap[entry.getAddress()]))) {
				cerr << "New entry will override old one..." << endl;
				*(entryMap[entry.getAddress()]) = entry;
			}
		} else {
			int randIdx = 0;
			if(!entryList.empty()) {
				// Get random number in [left, right].
				int left = 0, right = entryList.size() - 1;
				randIdx = rand() % (right - left + 1) + left;
			}
			cerr << "randIdx for insert: " << randIdx << endl;
			auto it = entryList.begin();		
			for(int i = 0; i < randIdx; i ++) {
				it ++;
			}
			entryList.insert(it, entry);
			// printList();
			entryMap[entry.getAddress()] = --it;
			// cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
			// printMap();
			// cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
		}
		return true;
	}

	// Get the ping target and advance lastPingEntryIdx.
	MembershipListEntry getPingTarget() {
		if(lastPingIt == entryList.end()) {
			// shuffle the whole list if we finish one round with it.
			shuffle_list<MembershipListEntry>(entryList.begin(), entryList.end());
			lastPingIt = entryList.begin();
		}
		auto rtn = *lastPingIt;
		lastPingIt ++;
		return rtn;
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
