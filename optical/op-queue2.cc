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
 */



#include "op-queue2.h"


void OpQueue2Handler::handle(Event*)
{
    op_queue_.resume();
}


static class OpQueue2Class : public TclClass {
public:
    OpQueue2Class() : TclClass("Queue/OpQueue2") {}
    TclObject* create(int, const char*const*) {
        return (new OpQueue2);
    }
} class_Op_Queue2;

void OpQueue2::reset()
{
    Queue::reset();
}

int OpQueue2::command(int argc, const char*const* argv) {

    if (argc==2) {
        if (strcmp(argv[1], "printstats") == 0) {
            print_summarystats();
            return (TCL_OK);
        }
    }
    if (argc == 3) {
        if (!strcmp(argv[1], "packetqueue-attach")) {
            delete q_;
            if (!(q_ = (PacketQueue*) TclObject::lookup(argv[2])))
                return (TCL_ERROR);
            else {
                pq_ = q_;
                return (TCL_OK);
            }
        }
    }
    return Queue::command(argc, argv);
}

/*
 * drop-tail
 */
void OpQueue2::enque(Packet* p)
{
    if (summarystats) {
        Queue::updateStats(qib_?q_->byteLength():q_->length());
    }

    int qlimBytes = qlim_ * mean_pktsize_;
    if ((!qib_ && (q_->length() + 1) >= qlim_) ||
            (qib_ && (q_->byteLength() + hdr_cmn::access(p)->size()) >= qlimBytes)) {
        // if the queue would overflow if we added this packet...
        if (drop_front_) { /* remove from head of queue */
            q_->enque(p);
            Packet *pp = q_->deque();
            drop(pp);
        } else {
            drop(p);
        }
    } else {
        q_->enque(p);
    }
}

Packet* OpQueue2::deque()
{
    if (summarystats && &Scheduler::instance() != NULL) {
        Queue::updateStats(qib_?q_->byteLength():q_->length());
    }
    return q_->deque();
}

void OpQueue2::print_summarystats()
{
    //double now = Scheduler::instance().clock();
    printf("True average queue: %5.3f", true_ave_);
    if (qib_)
        printf(" (in bytes)");
    printf(" time: %5.3f\n", total_time_);
}


void OpQueue2::recv(Packet* p, Handler*)
{

    hdr_burst* burstdh = hdr_burst::access(p);
// hdr_src* srh = hdr_src::access(p);
    hdr_cmn* comh = hdr_cmn::access(p);



    if(comh->ptype()==PT_OP_BURST) {
        //Tail Dropping
        burstdh->seg_type=1;

        target_->recv(p, &opqh_);
    }

}

void OpQueue2::resume()
{
//printf("OpQueue2::resume at %.15f\n",Scheduler::instance().clock());
// 	double now = Scheduler::instance().clock();
// 	Packet* p = deque();
// 	if (p != 0) {
// 		target_->recv(p, &opqh_);
// 	} else {
// 		if (unblock_on_resume_) {
// 			utilUpdate(last_change_, now, blocked_);
// 			last_change_ = now;
// 			blocked_ = 0;
// 		}
// 		else {
// 			utilUpdate(last_change_, now, blocked_);
// 			last_change_ = now;
// 			blocked_ = 1;
// 		}
// 	}
}



