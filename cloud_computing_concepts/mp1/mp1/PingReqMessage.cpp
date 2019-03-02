#include "PingReqMessage.h"

string PingReqMessage::getId() {
    return source.getAddress() + "|" + route.getAddress() + "|" + destination.getAddress() + "|" + to_string(protocol_period);
}