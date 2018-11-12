#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <climits>
#include <queue>
#include <string>
#include <deque>
#include <set>
#include <unordered_set>
#include <stack>

using namespace std;

template <typename T>
void printVec(const vector<T>& vec, const string& sep = " ") {
    for(auto& val : vec) {
        cout << val << sep;
    }
    cout << endl;
}

template <typename T, typename U>
void printVec(const vector<pair<T, U>>& vec, const string& sep = " ") {
    for(auto& p : vec) {
        cout << "(" << p.first << ", " << p.second << ")" << sep; 
    }
    cout << endl;
}

vector<string> split(string s, const string& delim) {
    vector<string> rtn;
    size_t pos = s.find(delim);
    while(pos != std::string::npos) {
        rtn.push_back(s.substr(0, pos));
        s = s.substr(pos + delim.size());
        pos = s.find(delim);
    }
    if(!s.empty()) {
        rtn.push_back(s);
    }
    return rtn;
} 