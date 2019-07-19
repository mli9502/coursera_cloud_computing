TODO:

7/3/2019:
- [x] For the node with address given by `Application::getjoinaddr`, it should put itself in membershipList.
- [x] Need to walk through the receive log and figure out what's the order of the the messages being received.

7/7/2019:
- [x] Add handle of `JoinResp` message.

7/16/2019:
- [x] When selecting Ping target, we need to exclude the current Node.
- [x] Need to go through the log. Preferably decrease the number of nodes first...

7/17/2019:
- [x] Send Ack msg when receiving a PingMsg.
- [ ] Start sending PingMsg on the start of a protocol period.
- [ ] Need to reset the protocolPeriod and protocolPeriodCounter when receiving JoinResp msg.

7/18/2019
- [ ] Need to read the paper again to understand the following cases:
    - Node is in the received membershipList, and also in curr node's failList.
    - Node is in the received failList, and also in curr node's membershipList.