#ifndef _ACK_MESSAGE_H_
#define _ACK_MESSAGE_H_

#include "BaseMessage.h"

class AckMessage : public BaseMessage {
public:
    unsigned long getIncarnation() {
        return incarnation;
    }
    string getId() override;

private:
    unsigned long incarnation;
};

#endif