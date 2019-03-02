#include "PingMessage.h"

string PingMessage::getId() {
    return source.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}


