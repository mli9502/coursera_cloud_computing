#include "PingReqMessage.h"

string PingReqMessage::getId() {
    return source.getAddress() + "|" + route.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingReqMessage::decode(string msg) {
    // TODO: fill this in.        
}

string PingReqMessage::encode() {
    // TODO: fill this in.
    return "";
}

void onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
}

