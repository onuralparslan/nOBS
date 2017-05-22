/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the MASH Research
 * 	Group at the University of California Berkeley.
 * 4. Neither the name of the University nor of the Research Group may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Contributed by Rishi Bhargava <rishi_bhargava@yahoo.com> May, 2001.
 */


#ifndef _OPSRAGENT_H
#define _OPSRAGENT_H

#include <stdarg.h>

#include "object.h"
#include "agent.h"
#include "trace.h"
#include "packet.h"
#include "priqueue.h"
#include "mac.h"
#include "../src_rtg/hdr_src.h"
#include "op-burst_agent.h"
#include "op-schedule.h"
#include "op-converterschedule.h"
#include "op-fdlschedule.h"
#include "opticaldefaults.h"

#define SEND_TIMEOUT 30.0	// # seconds a packet can live in sendbuf
#define SEND_BUF_SIZE 64


//Maximum number of destinations
#define MAX_DEST OpticalDefaults::instance()->MAX_DEST

//type of JET reservation. Set to 0 for JET reservation scheme shown in Fig. 1.a, where HOP_DELAY includes SWITCHTIME, 
//and 1 for JET reservation scheme shown in Fig. 1.b, where HOP_DELAY does not include SWITCHTIME so lower HOP_DELAY can be selected, as in the paper
//C. Qiao and M. Yoo, “Optical Burst Switching (OBS) - A New Paradigm for an Optical Internet”, J. High. Speed Networks, Vol. 8, No. 1, pp. 69-84, Jan. 1999
#define JET_TYPE OpticalDefaults::instance()->JET_TYPE

//minumum time difference between two bursts for switching
#define SWITCHTIME OpticalDefaults::instance()->SWITCHTIME

class Agent;


class OpSRAgent : public Agent {
public:

    // in the table the first entry is the number of hops... then follows the path

    hash_node **table;



    void install(int slot, NsObject*);
    void alloc(int);
    NsObject **slot_;
    int nslot_;
    int maxslot_;

    virtual int command(int argc, const char*const* argv);
    virtual void recv(Packet*, Handler* callback = 0);


    OpSRAgent();
    ~OpSRAgent();
    

private:

//   int off_src_;

    int slotnumber;
    OpSchedule *LinkReservation_;
    OpConverterSchedule *ConverterReservation_;
    OpFDLSchedule *FDLReservation_;

//   Agent *agent_;

//   int nodenum_;
    //list of optical nodes
    bool *opticnodes;


//   int nodeNum_;
    int nodetype_;
    int converternumber_;
    int fdlnumber_;
    int conversiontype_;
    int ackdontburst;
    
    Trace           **logtarget;
    Trace           **logtargetnam;


};

#endif // _SRAGENT_H
