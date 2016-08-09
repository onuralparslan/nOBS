/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 1997 The Regents of the University of California.
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



//
// a generalized classifier for mapping (src/dest/flowid) fields
// to a bucket.  "buckets_" worth of hash table entries are created
// at init time, and other entries in the same bucket are created when
// needed
//
//

extern "C" {
#include <tcl.h>
}

#include <stdlib.h>
#include "config.h"
#include "packet.h"
#include "ip.h"
#include "op-classifier.h"
#include "op-classifier-hash.h"

/****************** HashClassifier Methods ************/

int OpHashClassifier::classify(Packet * p) {
    printf("OpHashClassifier::classify at %f\n",Scheduler::instance().clock());
    int slot= lookup(p);
    if (slot >= 0 && slot <=maxslot_)
        return (slot);
    else if (default_ >= 0)
        return (default_);
    return (unknown(p));
} // HashClassifier::classify

int OpHashClassifier::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();
    /*
     * $classifier set-hash $hashbucket src dst fid $slot
     */

    if (argc == 7) {
        if (strcmp(argv[1], "set-hash") == 0) {
            //xxx: argv[2] is ignored for now
            nsaddr_t src = atoi(argv[3]);
            nsaddr_t dst = atoi(argv[4]);
            int fid = atoi(argv[5]);
            int slot = atoi(argv[6]);
            if (0 > set_hash(src, dst, fid, slot))
                return TCL_ERROR;
            return TCL_OK;
        }
    } else if (argc == 6) {
        /* $classifier lookup $hashbuck $src $dst $fid */
        if (strcmp(argv[1], "lookup") == 0) {
            nsaddr_t src = atoi(argv[3]);
            nsaddr_t dst = atoi(argv[4]);
            int fid = atoi(argv[5]);
            int slot= get_hash(src, dst, fid);
            if (slot>=0 && slot <=maxslot_) {
                tcl.resultf("%s", slot_[slot]->name());
                return (TCL_OK);
            }
            tcl.resultf("");
            return (TCL_OK);
        }
        // Added by Yun Wang to set rate for TBFlow or TSWFlow
        if (strcmp(argv[1], "set-flowrate") == 0) {
            int fid = atoi(argv[2]);
            nsaddr_t src = 0;  // only use fid
            nsaddr_t dst = 0;  // to classify flows
            int slot = get_hash( src, dst, fid );
            if ( slot >= 0 && slot <= maxslot_ ) {
                Flow* f = (Flow*)slot_[slot];
                tcl.evalf("%u set target_rate_ %s",
                          f, argv[3]);
                tcl.evalf("%u set bucket_depth_ %s",
                          f, argv[4]);
                tcl.evalf("%u set tbucket_ %s",
                          f, argv[5]);
                return (TCL_OK);
            }
            else {
                tcl.evalf("%s set-rate %u %u %u %u %s %s %s",
                          name(), src, dst, fid, slot, argv[3], argv[4],argv[5])
                ;
                return (TCL_OK);
            }
        }
    } else if (argc == 5) {
        /* $classifier del-hash src dst fid */
        if (strcmp(argv[1], "del-hash") == 0) {
            nsaddr_t src = atoi(argv[2]);
            nsaddr_t dst = atoi(argv[3]);
            int fid = atoi(argv[4]);

            Tcl_HashEntry *ep= Tcl_FindHashEntry(&ht_,
                                                 hashkey(src, dst,
                                                         fid));
            if (ep) {
                long slot = (long)Tcl_GetHashValue(ep);
                Tcl_DeleteHashEntry(ep);
                tcl.resultf("%lu", slot);
                return (TCL_OK);
            }
            return (TCL_ERROR);
        }
    }
    return (OpClassifier::command(argc, argv));
}

/**************  TCL linkage ****************/
static class OpSrcDestHashClassifierClass : public TclClass {
public:
    OpSrcDestHashClassifierClass() : TclClass("OpClassifier/OpHash/OpSrcDest") {}
    TclObject* create(int, const char*const*) {
        return new OpSrcDestHashClassifier;
    }
} class_op_hash_srcdest_classifier;

static class OpFidHashClassifierClass : public TclClass {
public:
    OpFidHashClassifierClass() : TclClass("OpClassifier/OpHash/OpFid") {}
    TclObject* create(int, const char*const*) {
        return new OpFidHashClassifier;
    }
} class_op_hash_fid_classifier;

static class OpDestHashClassifierClass : public TclClass {
public:
    OpDestHashClassifierClass() : TclClass("OpClassifier/OpHash/OpDest") {}
    TclObject* create(int, const char*const*) {
        return new OpDestHashClassifier;
    }
} class_op_hash_dest_classifier;

static class OpSrcDestFidHashClassifierClass : public TclClass {
public:
    OpSrcDestFidHashClassifierClass() : TclClass("OpClassifier/OpHash/OpSrcDestFid") {}
    TclObject* create(int, const char*const*) {
        return new OpSrcDestFidHashClassifier;
    }
} class_op_hash_srcdestfid_classifier;


// DestHashClassifier methods
int OpDestHashClassifier::classify(Packet *p)
{
    printf("OpDestHashClassifier::classify at %f\n",Scheduler::instance().clock());
    int slot= lookup(p);
    if (slot >= 0 && slot <=maxslot_)
        return (slot);
    else if (default_ >= 0)
        return (default_);
    return -1;
} // HashClassifier::classify

void OpDestHashClassifier::do_install(char* dst, NsObject *target) {
    nsaddr_t d = atoi(dst);
    int slot = getnxt(target);
    install(slot, target);
    if (set_hash(0, d, 0, slot) < 0)
        fprintf(stderr, "OpDestHashClassifier::set_hash from within OpDestHashClassifier::do_install returned value < 0");
}

int OpDestHashClassifier::command(int argc, const char*const* argv)
{
    if (argc == 4) {
        // $classifier install $dst $node
        if (strcmp(argv[1], "install") == 0) {
            char dst[SMALL_LEN];
            strcpy(dst, argv[2]);
            NsObject *node = (NsObject*)TclObject::lookup(argv[3]);
            //nsaddr_t dst = atoi(argv[2]);
            do_install(dst, node);
            return TCL_OK;
            //int slot = getnxt(node);
            //install(slot, node);
            //if (set_hash(0, dst, 0, slot) >= 0)
            //return TCL_OK;
            //else
            //return TCL_ERROR;
        } // if
    } else if(argc == 3)
        if (strcmp(argv[1], "install-burst-agent") == 0) {
            NsObject *node = (NsObject*)TclObject::lookup(argv[2]);
            BurstAgent_ = node;
            return (TCL_OK);
        }
    return(OpHashClassifier::command(argc, argv));
} // command


void OpDestHashClassifier::recv(Packet* p, Handler*h)
{

    hdr_ip* iph = hdr_ip::access(p);
    hdr_cmn* comh = hdr_cmn::access(p);
    int source_addr = mshift(iph->saddr());
    int dest_addr = mshift(iph->daddr());

    printf("OpDestHashClassifier::recv type %d at address_ %d source %d destination %d flowid %d at %f\n",comh->ptype(),address_,source_addr,dest_addr,iph->flowid(), Scheduler::instance().clock());


    if((comh->ptype()!=PT_OP_BURST)&&(source_addr==address_)) {
        printf("OpDestHashClassifier::recv burst sent\n");

        BurstAgent_->recv(p,h);
//NsObject* node = find(p);
//node->recv(p,h);

    } else if((comh->ptype()==PT_OP_BURST)&&(dest_addr==address_)) {
        printf("OpDestHashClassifier::recv burst sent\n");

        BurstAgent_->recv(p,h);
//NsObject* node = find(p);
//node->recv(p,h);
    } else {
        NsObject* node = find(p);
        if (node == NULL) {
            /*
             * XXX this should be "dropped" somehow.  Right now,
             * these events aren't traced.
             */
            printf("\n!!!!OpDestHashClassifier::recv drop\n");
            Packet::free(p);
            return;
        }
        printf("OpDestHashClassifier::recv packet sent\n");
        node->recv(p,h);
    }

}

void OpHashClassifier::set_table_size(int)
{}
