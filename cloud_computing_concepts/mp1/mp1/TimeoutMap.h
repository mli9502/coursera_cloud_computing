#ifndef _TIMEOUT_MAP_H_
#define _TIMEOUT_MAP_H_

#include <iostream>
#include <map>
#include <vector>
#include <utility>

using namespace std;

// TODO: @mli: 2/27/2019: Need to implement this properly as a container. Need to learn how to implement a container.

/**
 * TimeoutMap defines a Map that supports timeout functionality.
 * When the tick() method is called, all the entries that exceeds the given timeout value will be removed from map.
 * 
 * This class is used for recording the ID for PING, PING_REQ and ACK we sent and received, 
 * because we have to timeout the request/response if we have exceed our period.
 */ 

template <typename K, typename V>
class TimeoutMap {
public:
    TimeoutMap(int ttl) : ttl(ttl) {}
    TimeoutMap(const vector<pair<K, V>>& list, int ttl) : ttl(ttl) {
        for(const auto& p : list) {
            internal[p.first] = {p.second, 0};
        }
    }
    ~TimeoutMap() {}
    
    // Check if key is in the map.
    bool contains(const K& key);
    // Insert the k-v pair into the map. 
    // no-op if key already exists.
    void insert(const K& key, const V& val);
    // Update the val for the given key.
    // no-op if key is not in the map.
    void update(const K& key, const V& val, bool resetTTL = false);
    // Get the value with key.
    V* get(const K& key);

    // Erase the given key from map.
    // no-op if key does not exist.
    void erase(const K& key);
    // Increase all the timer. Evict timeout entries.
    void tick();

    void print();

private:
    // the underlying datastructure.
    map<K, pair<V, int>> internal;
    // time-to-live.
    unsigned ttl;
};

template <typename K, typename V>
bool TimeoutMap<K, V>::contains(const K& key) {
    return internal.find(key) != internal.end();
}

template <typename K, typename V>
void TimeoutMap<K, V>::insert(const K& key, const V& val) {
    if(contains(key)) {
#ifdef DEBUGLOG
        cout << "TimeoutMap::insert: key: " << key << " already exists..." << endl; 
#endif
        return;
    }
    internal[key] = {val, 0};
}

template <typename K, typename V>
void TimeoutMap<K, V>::update(const K& key, const V& val, bool resetTTL) {
    if(!contains(key)) {
#ifdef DEBUGLOG
        cout << "TimeoutMap::update: key: " << key << " does not exist..." << endl;
#endif        
        return;
    }
    internal[key].first = val;
    if(resetTTL) {
        internal[key].second = 0;
    }
}

template <typename K, typename V>
V* TimeoutMap<K, V>::get(const K& key) {
    if(!contains(key)) {
        return nullptr;
    }
    return &(internal[key].first);
}

template <typename K, typename V>
void TimeoutMap<K, V>::erase(const K& key) {
    if(!contains(key)) {
#ifdef DEBUGLOG
        cout << "TimeoutMap::erase: key: " << key << " does not exist..." << endl;
#endif        
        return;
    }
    internal.erase(key);
}

template <typename K, typename V>
void TimeoutMap<K, V>::tick() {
    for(auto& entry : internal) {
        entry.second.second ++;
        if(entry.second.second >= ttl) {
#ifdef DEBUGLOG
            cout << "evict entry: [" << entry.first << ": " << "(" << entry.second.first << ", " << entry.second.second << ")]" << endl;
#endif            
            erase(entry.first);
        }
    }
}

// TODO: Need to either fix this, or remove this.
// currently, V has to be type Address...
template <typename K, typename V>
void TimeoutMap<K, V>::print() {
    cout << "##### ##### #####" << endl;
    for(auto& entry : internal) {
        cout << entry.first << ": " << "(" << entry.second.first.getAddress() << ", " << entry.second.second << ")" << endl;
    }
}

#endif