#ifndef _PING_MESSAGE_H_
#define _PING_MESSAGE_H_

#include "BaseMessage.h"

class PingMessage : public BaseMessage {
public:
    string getId() override;    
};

#endif