// Copyright (c) 2000 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//




#ifndef ns_op_burst_h
#define ns_op_burst_h

#include <fstream>
#include <timer.h>
#include "agent.h"
#include "tclcl.h"
#include "packet.h"
#include "address.h"
#include "ip.h"
#include "tcp.h"
#include "../src_rtg/hdr_src.h"

#include "opticaldefaults.h"

//Burst is sent when the total number of packets inside the burst reaches this threshold.
#define MAX_PACKET_NUM OpticalDefaults::instance()->MAX_PACKET_NUM

//Burst is sent when the total size of packets (in terms of Bytes) inside the burst passes this threshold.
#define BURST_SIZE_THRESHOLD OpticalDefaults::instance()->BURST_SIZE_THRESHOLD

//node processing time
#define HOP_DELAY OpticalDefaults::instance()->HOP_DELAY

//burst sending timeout
#define TIMEOUT OpticalDefaults::instance()->TIMEOUT

//wavelegth per link
#define MAX_LAMBDA OpticalDefaults::instance()->MAX_LAMBDA

//wavelength speed
#define LINKSPEED OpticalDefaults::instance()->LINKSPEED

//minumum time difference between two bursts for switching
#define SWITCHTIME OpticalDefaults::instance()->SWITCHTIME

//output information
#define DEBUG OpticalDefaults::instance()->DEBUG

//number of nodes
#define MAX_DEST OpticalDefaults::instance()->MAX_DEST

//Burst header size
#define BURST_HEADER OpticalDefaults::instance()->BURST_HEADER

//max. number of (full size) bursts delayed in the reservation
#define MAX_DELAYED_BURST OpticalDefaults::instance()->MAX_DELAYED_BURST

//maximum number of burst queues for flows
#define MAX_FLOW_QUEUE OpticalDefaults::instance()->MAX_FLOW_QUEUE

//minumum time difference between two bursts for FDL switching. Normally equal to SWITCHTIME
#define FDLSWITCHTIME OpticalDefaults::instance()->FDLSWITCHTIME

//FDL type
#define FDLBURSTCAP OpticalDefaults::instance()->FDLBURSTCAP

//Maximum MTU size
#define MAX_MTU OpticalDefaults::instance()->MAX_MTU

//type of JET reservation. Set to 0 for JET reservation scheme shown in Fig. 1.a, where HOP_DELAY includes SWITCHTIME, 
//and 1 for JET reservation scheme shown in Fig. 1.b, where HOP_DELAY does not include SWITCHTIME so lower HOP_DELAY can be selected, as in the paper
//C. Qiao and M. Yoo, “Optical Burst Switching (OBS) - A New Paradigm for an Optical Internet”, J. High. Speed Networks, Vol. 8, No. 1, pp. 69-84, Jan. 1999
#define JET_TYPE OpticalDefaults::instance()->JET_TYPE


class BurstAgent;


class SendTmr : public TimerHandler {
public:
    SendTmr() : TimerHandler() {}
    void Tmr_Init(BurstAgent *a, int i, int j)  {
        a_ = a;
        dest_=i;
        queue_=j;
    }


protected:
    virtual void expire(Event *e);
    BurstAgent *a_;
    int dest_;
    int queue_;
};


struct hdr_burst {

    //the time burst was sent
    double send_time;

    //the time burst was created
    double created_time;

    //size of the burst
    u_int burst_size;

    //number of packets inside the burst
    u_int packet_num;

    //Type of the burst packet. Currently 0 for control, 1 for data packet
    u_int burst_type;

    //Type of dropping. Currently 0 for head, 1 for tail
    u_int seg_type;

    //id number of the burst
    u_long burst_id;

    //length of the route left
    int route_length;

    //total length of the route
    int route_length_tot;

    //amount of delay in the first reservation
    double delayedresv;

    unsigned int linkspeed;

    int lambda;

    int source;
    int destination;

    int flow;

    //just for this dirty simulation. delete it after!!!!!!!!!!!!!!!!!
    int ack;

    //this is the first link that the control packet will pass
    int first_link;

    int drop;

    //pointer to data burst
    Packet *burst;




    // Header access methods
    static int offset_; // required by PacketHeaderManager
    inline static int& offset() {
        return offset_;
    }
    inline static hdr_burst* access(const Packet* p) {
        return (hdr_burst*) p->access(offset_);
    }
};

class BurstAgent : public Agent {
public:
    BurstAgent();
    ~BurstAgent();
    virtual int command(int argc, const char*const* argv);
    virtual void recv(Packet*, Handler*);
    void timeoutsend(int dest, int queue);
    void burstsend(int dest, int flow);



    SendTmr** Tmr_;

    double Timeout;

    int nodenum_;
    int **flowshist;



private:
    Packet*** burst_data;
    Packet*** burst_cont;

    Packet**** burst_buffer;

    bool *opticnodes;

    FILE* stat;
    unsigned int record;
    int ackdontburst;
    
    u_long id;


};

#endif // ns_burst_h
