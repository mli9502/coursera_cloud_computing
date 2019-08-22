#ifndef _UTIL_H_
#define _UTIL_H_

#include <cstring>

template <typename T>
static void copyMsg(char*& msg, T field) {
    memcpy(msg, &field, sizeof(T));
    msg += sizeof(T);
}

template <typename T>
static void copyObj(char const *& msg, T& field) {
    memcpy(&field, msg, sizeof(T));
    msg += sizeof(T);
}

#endif // _UTIL_H_

