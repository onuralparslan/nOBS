/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1994 Regents of the University of California.
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
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
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
 *
 */

#ifndef ns_op_fdlqueue_h
#define ns_op_fdlqueue_h

#include <string.h>
#include "queue.h"
#include "config.h"
#include "../src_rtg/hdr_src.h"
#include "op-burst_agent.h"
#include "packet.h"
#include "ip.h"







class OpFDLSchedule;


struct FDLSchList
{
    Packet *p;
    FDLSchList* Next;

    double start;
    double end;


    u_long burst_id;
    int source;
    int destination;
    int flow;


};




class OpFDLScheduleHandler : public Handler {
public:
    inline OpFDLScheduleHandler(OpFDLSchedule& q) : op_fdlschedule_(q) {}
    void handle(Event*);
private:
    OpFDLSchedule& op_fdlschedule_;
};


class OpFDLSchedule  {
public:
    OpFDLSchedule(int fdlnumber_): opsh_(*this) {
        Head = new FDLSchList*[fdlnumber_];
        Tail = new FDLSchList*[fdlnumber_];
        CurrentPtr = new FDLSchList*[fdlnumber_];

        fdl_size= new double[fdlnumber_];

        for(int i=0; i<fdlnumber_; i++) {

            Head[i] = new FDLSchList();
            Tail[i]=Head[i];
            CurrentPtr[i] = Head[i];

            Head[i]->Next=NULL;
            Head[i]->p=NULL;
            Head[i]->start=-1;
            Head[i]->end=-1;
            Head[i]->burst_id=-1;
            Head[i]->source=-1;
            Head[i]->destination=-1;
            fdl_size[i]=i;
        }

        fdlnumber=fdlnumber_;

    }
    ~OpFDLSchedule() {

        for(int i=0; i<fdlnumber; i++) {

            delete Head[i];
            delete Tail[i];
            delete CurrentPtr[i];

        }

    }
    double recv(Packet* p, int action, int fdlnumber);
    void resume();
    void init(int slot);
    int slotnumber;

    void fdlsize(double size, int number);

protected:


    double* fdl_size;
    OpFDLScheduleHandler opsh_;
    int compare(double a, double b);
    int compare2(double a, double b);


    double ScheduleBurst(Packet* p, int action, int fdlnumber);
    FDLSchList* Previous(long index);

    void ScheduleBurst2(Packet* p_, double min_gap, int bestlambda, int k, int is_head, int delaystop, double arrival, double leave, double burst_duration, double switch_time, double linkspeed, double realarrival, double maximum_delay,FDLSchList* temp, FDLSchList* temp2, FDLSchList* besttemp);



    FDLSchList** Head;
    FDLSchList** Tail;
    FDLSchList** CurrentPtr;

    clock_t started;
    clock_t ended;

    int fdlnumber;

};

#endif
