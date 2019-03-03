#include "PingMessage.h"

string PingMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}

void PingMessage::decode(const string& msg) {
    // TODO: fill this in.
}

string PingMessage::encode() {
    // TODO: fill this in.
    return "";
}

void onReceiveHandler(MP1Node& state) {
    // TODO: fill this in.
}