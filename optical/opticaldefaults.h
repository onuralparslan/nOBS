// Copyright (c) 2004 by Bilkent University
// All rights reserved.
#ifndef optical_defaults_h
#define optical_defaults_h
#include "object.h"
#include "tcp-sink.h"
class OpticalDefaults;

class OpticalDefaults : public TclObject {
public:
    int MAX_PACKET_NUM;
    int BURST_SIZE_THRESHOLD;
    double HOP_DELAY;
    double TIMEOUT;
    int MAX_LAMBDA;
    double LINKSPEED;
    double SWITCHTIME;
    int DEBUG;
    int MAX_DEST;
    int BURST_HEADER;
    int MAX_DELAYED_BURST;
    int MAX_MTU;
    int MAX_FLOW_QUEUE;
    double FDLSWITCHTIME;
    int FDLBURSTCAP;
    int JET_TYPE;


    OpticalDefaults() {
        bind("MAX_PACKET_NUM",&MAX_PACKET_NUM);
        bind("BURST_SIZE_THRESHOLD",&BURST_SIZE_THRESHOLD);
        bind_time("HOP_DELAY",&HOP_DELAY);
        bind_time("TIMEOUT",&TIMEOUT);
        bind("MAX_LAMBDA",&MAX_LAMBDA);
        bind_bw("LINKSPEED",&LINKSPEED);
        bind_time("SWITCHTIME",&SWITCHTIME);
        bind("DEBUG",&DEBUG);
        bind("MAX_DEST",&MAX_DEST);
        bind("BURST_HEADER",&BURST_HEADER);
        bind("MAX_DELAYED_BURST",&MAX_DELAYED_BURST);
        bind("MAX_FLOW_QUEUE",&MAX_FLOW_QUEUE);
        bind_time("FDLSWITCHTIME",&FDLSWITCHTIME);
        bind("FDLBURSTCAP",&FDLBURSTCAP);
        bind("JET_TYPE",&JET_TYPE);
        bind("MAX_MTU",&MAX_MTU);
        instance_ = this;
    }

    inline static OpticalDefaults* instance() {
        if (OpticalDefaults::instance_)
            return OpticalDefaults::instance_;
        return new OpticalDefaults;
    }
private:
    //Stores the instance of the last created optical defaults object.
    static OpticalDefaults* instance_;
};

#endif // optical_defaults_h
