#ifndef ns_op_nullagent_h
#define ns_op_nullagent_h

#include "agent.h"
#include "op-burst_agent.h"

class OpticalNullAgent : public Agent {
public:
    OpticalNullAgent() : Agent(PT_OP_BURST) {}
    virtual void recv(Packet* pkt, Handler*) {
        hdr_burst* bursth = hdr_burst::access(pkt);
        Packet **p = (Packet**) pkt->accessdata();
        for(unsigned int i=0; i<bursth->packet_num; i++) {
            drop(*p);
            p++;
        }
        drop(pkt);
    }
};

#endif
