#ifndef _PING_REQ_MESSAGE_H_
#define _PING_REQ_MESSAGE_H_

#include "BaseMessage.h"

class PingReqMessage : public BaseMessage {
public:
    Address getRoute() {
        return route;
    }
    string getId() override;

private:
    Address route;
};

#endif