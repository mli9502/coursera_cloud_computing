#include "AckMessage.h"

string AckMessage::getId() {
    return destination.getAddress() + "|" + source.getAddress() + "|" + to_string(protocol_period);
}