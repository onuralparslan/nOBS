/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1996-1997 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 */


#include "opticaldefaults.h"
#include "op-delay.h"
#include "mcast_ctrl.h"
#include "ctrMcast.h"

static class OpLinkDelayClass : public TclClass {
public:
    OpLinkDelayClass() : TclClass("OpDelayLink") {}
    TclObject* create(int /* argc */, const char*const* /* argv */) {
        return (new OpLinkDelay);
    }
} class_op_delay_link;

OpLinkDelay::OpLinkDelay() : dynamic_(0), latest_time_(0), itq_(0)
{
    bind_bw("bandwidth_", &bandwidth_);
    bind_time("delay_", &delay_);
    bind_bool("avoidReordering_", &avoidReordering_);
}

int OpLinkDelay::command(int argc, const char*const* argv)
{
    if (argc == 2) {
        if (strcmp(argv[1], "isDynamic") == 0) {
            dynamic_ = 1;
            itq_ = new PacketQueue();
            return TCL_OK;
        }
    } else if (argc == 6) {
        if (strcmp(argv[1], "pktintran") == 0) {
            int src = atoi(argv[2]);
            int grp = atoi(argv[3]);
            int from = atoi(argv[4]);
            int to = atoi(argv[5]);
            pktintran (src, grp);
            Tcl::instance().evalf("%s puttrace %d %d %d %d %d %d %d %d", name(), total_[0], total_[1], total_[2], total_[3], src, grp, from, to);
            return TCL_OK;
        }
    }

    return Connector::command(argc, argv);
}

void OpLinkDelay::recv(Packet* p, Handler* h)
{


    double txt=txtime(p);
//printf("OpLinkDelay!!!!!!!!! \n");
    hdr_burst* burstdh = hdr_burst::access(p);
    if(DEBUG==1) printf("OpLinkDelay::recv type %d burstdh->route_length %d at %.15f\n",hdr_burst::access(p)->burst_type, burstdh->route_length, Scheduler::instance().clock());
    burstdh->route_length=burstdh->route_length-1;
    if(DEBUG==1) printf("afOpLinkDelay::recv type %d burstdh->route_length %d at %.15f\n",hdr_burst::access(p)->burst_type, burstdh->route_length, Scheduler::instance().clock());

    Scheduler& s = Scheduler::instance();
    if (dynamic_) {
        Event* e = (Event*)p;
        e->time_= txt + delay_;
        itq_->enque(p); // for convinience, use a queue to store packets in transit
        s.schedule(this, p, txt + delay_);

    } else if (avoidReordering_) {
        // code from Andrei Gurtov, to prevent reordering on
        //   bandwidth or delay changes
        double now_ = Scheduler::instance().clock();
        if (txt + delay_ < latest_time_ - now_ && latest_time_ > 0) {
            latest_time_+=txt;
            s.schedule(target_, p, latest_time_ - now_ );
        } else {
            latest_time_ = now_ + txt + delay_;
            s.schedule(target_, p, txt + delay_);
        }

    } else {
        //printf("OpLinkDelay::recv txt + delay_ %.15f at %.15f\n",txt + delay_,Scheduler::instance().clock());

        if(burstdh->burst_type==0) {
            //optical control packet

            if(burstdh->first_link==1) {
                //it is the first link
                if(DEBUG==1) printf("OpLinkDelay::recv first link of control packet unique id %d delay_ %.15f arrive %.15f\n",hdr_cmn::access(p)->uid_, delay_,delay_+NOW);
                burstdh->first_link=0;
                s.schedule(target_, p, delay_);

            } else {
                //not the first link
                if(DEBUG==1) printf("OpLinkDelay::recv not first link of control packet unique id %d delay_+HOP_DELAY %.15f delay_ %.15f HOP_DELAY %.15f arrive %.15f\n",hdr_cmn::access(p)->uid_, delay_+HOP_DELAY,delay_,HOP_DELAY,delay_+HOP_DELAY+NOW);
                s.schedule(target_, p, delay_+HOP_DELAY);


            }
        } else {
            //optical burst packet

            //Arrivals are based on tail arrival in both cases.
            if(burstdh->first_link==1) {
                //it is the first link
                burstdh->first_link=0;
                if(DEBUG==1) printf("OpLinkDelay::recv first link of burst packet unique id %d delay_+txtime(p) %.15f delay_ %.15f txtime(p) %.15f arrive %.15f\n",hdr_cmn::access(p)->uid_, delay_+txtime(p),delay_,txtime(p),delay_+txtime(p)+NOW);
                s.schedule(target_, p, delay_+txtime(p));

            } else {
                //not the first link
                if(DEBUG==1) printf("OpLinkDelay::recv first link of burst packet unique id %d delay_ %.15f arrive %.15f\n",hdr_cmn::access(p)->uid_, delay_,delay_+NOW);
                s.schedule(target_, p, delay_);
            }

        }
    }
    //printf("OpLinkDelay::recv txt %.15f at %.15f\n",txt,Scheduler::instance().clock());
//	s.schedule(h, &intr_[0], txt);
}

void OpLinkDelay::send(Packet* p, Handler*)
{
//printf("OpLinkDelay::send at %.15f\n",Scheduler::instance().clock());
    target_->recv(p, (Handler*) NULL);
}

void OpLinkDelay::reset()
{
    Scheduler& s= Scheduler::instance();

    if (itq_ && itq_->length()) {
        Packet *np;
        // walk through packets currently in transit and kill 'em
        while ((np = itq_->deque()) != 0) {
            s.cancel(np);
            drop(np);
        }
    }
}

void OpLinkDelay::handle(Event* e)
{
//printf("OpLinkDelay::handle at %.15f\n",Scheduler::instance().clock());
    Packet *p = itq_->deque();
    assert(p->time_ == e->time_);
    send(p, (Handler*) NULL);
}

void OpLinkDelay::pktintran(int src, int group)
{
    int reg = 1;
    int prune = 30;
    int graft = 31;
    int data = 0;
    for (int i=0; i<4; i++) {
        total_[i] = 0;
    }

    if (! dynamic_)
        return;

    int len = itq_->length();
    while (len) {
        len--;
        Packet* p = itq_->lookup(len);
        hdr_ip* iph = hdr_ip::access(p);
        if (iph->flowid() == prune) {
            if (iph->saddr() == src && iph->daddr() == group) {
                total_[0]++;
            }
        } else if (iph->flowid() == graft) {
            if (iph->saddr() == src && iph->daddr() == group) {
                total_[1]++;
            }
        } else if (iph->flowid() == reg) {
            hdr_CtrMcast* ch = hdr_CtrMcast::access(p);
            if (ch->src() == src+1 && ch->group() == group) {
                total_[2]++;
            }
        } else if (iph->flowid() == data) {
            if (iph->saddr() == src+1 && iph->daddr() == group) {
                total_[3]++;
            }
        }
    }
    //printf ("%.15f %d %d %d %d\n", Scheduler::instance().clock(), total_[0], total_[1], total_[2],total_[3]);
}

