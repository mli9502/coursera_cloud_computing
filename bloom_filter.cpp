#include "util.h"

// return the value of ith hash function.
// hash_i = ((x^2 + x^3) * i) % bits.
int hash_i(long val, long i, int bits) {
    long tmp = (val * val + val * val * val) * (i + 1);
    // cout << tmp << endl; 
    return tmp % bits;
}

// insert element into bloom filter.
void insertElement(int val, vector<int>& bitMap) {
    for(int i = 0; i < 3; i ++) {
        int idx = hash_i(val, i, 32);
        cout << idx << endl;
        bitMap[idx] = 1;
    }
}

int main(void) {
    vector<int> bitMap(32, 0);
    insertElement(2013, bitMap);
    insertElement(2010, bitMap);
    insertElement(2007, bitMap);
    insertElement(2004, bitMap);
    // insertElement(2001, bitMap);
    // insertElement(1998, bitMap);
    printVec(bitMap);
    insertElement(3200, bitMap);
    printVec(bitMap);
}