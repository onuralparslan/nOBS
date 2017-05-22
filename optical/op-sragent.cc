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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <float.h>

#include "object.h"
#include "agent.h"
#include "trace.h"
#include "packet.h"
#include "scheduler.h"
#include "random.h"

#include "mac.h"
#include "ll.h"
#include "cmu-trace.h"

#include "op-sragent.h"
#include <fstream>




/*===========================================================================
  SRAgent OTcl linkage
---------------------------------------------------------------------------*/
static class OpSRAgentClass : public TclClass {
public:
    OpSRAgentClass() : TclClass("Agent/OpSRAgent") {}
    TclObject* create(int, const char*const*) {
        return (new OpSRAgent);
    }
} class_OpSRAgent;

/*===========================================================================
  SRAgent methods
---------------------------------------------------------------------------*/
OpSRAgent::OpSRAgent():Agent(PT_TCP), slot_(0), nslot_(0), maxslot_(-1)
{
    //  cout << " into the constructor " << endl;

    slotnumber=0;


    bind("nodetype_", &nodetype_);
    bind("converternumber_", &converternumber_);
    bind("fdlnumber_", &fdlnumber_);
    bind("conversiontype_", &conversiontype_);
    bind("ackdontburst", &ackdontburst);


    logtarget = new Trace*[MAX_DEST];
    logtargetnam = new Trace*[MAX_DEST];
    for(int i=0; i<MAX_DEST; i++) {
            logtarget[i]=NULL;
            logtargetnam[i]=NULL;
        }
    

    opticnodes= new bool[MAX_DEST];

    table = new h_node*[MAX_DEST];

    //setup optical nodes list
    for (int tmp=0; tmp < MAX_DEST; tmp++) {
        opticnodes[tmp] = 0;
    }


}

OpSRAgent::~OpSRAgent()
{
    delete [] slot_;
    delete [] opticnodes;
    delete [] LinkReservation_;
    delete [] logtarget;
    delete [] logtargetnam;
    delete ConverterReservation_;
}



int
OpSRAgent::command(int argc, const char*const* argv)
{
    Tcl& tcl = Tcl::instance();

    if(argc == 2)
    {
        if (strcmp(argv[1],"reset") == 0)
        {
            tcl.resultf("reset done");
            return(TCL_OK);
        }
        if (strcmp(argv[1],"create") == 0)
        {
            LinkReservation_ = new OpSchedule[MAX_DEST];
            for(int i=0; i<MAX_DEST; i++) {
                LinkReservation_[i].init(i);
                //printf("LinkReservation_[i].slotnumber %d\n",LinkReservation_[i].slotnumber);
            }
            ConverterReservation_ = new OpConverterSchedule(converternumber_);
	    if(DEBUG==1) printf("OpSRAgent converternumber_ %d\n",converternumber_);

            FDLReservation_ = new OpFDLSchedule(fdlnumber_);
	    if(DEBUG==1) printf("OpSRAgent fdlnumber_ %d\n",fdlnumber_);

            return(TCL_OK);
        }
    }

    else if(argc == 3)
    {
        if((strcmp(argv[1], "target") == 0))
        {
            target_ = (NsObject *) TclObject::lookup(argv[2]);
            if (target_ == 0) {
	      printf("OpSource target_ error\n");
                tcl.resultf("no such target %s", argv[2]);
                return(TCL_ERROR);
            }
            //printf("OpSource target_ %d\n",target_);
            tcl.resultf(" The target is successfully set");
            return(TCL_OK);
        }
        
        
    }

    else if(argc == 4)
    {
        if((strcmp(argv[1], "install_slot") == 0))
        {
            //cout << " Installing " << argv[2] << " in " << argv[3] << endl;
            NsObject* obj = (NsObject*)TclObject::lookup(argv[2]);
            //printf("OpSource installslot for %d obj %d\n",atoi(argv[3]),obj);
            if (obj == 0)
            {
                tcl.resultf("SRAgent: %s lookup of %s failed\n", argv[1], argv[2]);
                return TCL_ERROR;
            }
            else
            {
                install(atoi(argv[3]), obj);
                return TCL_OK;
            }

        } else if(strcmp(argv[1], "add_trace_target") == 0) {
            logtarget[atoi(argv[3])] = (Trace*) TclObject::lookup(argv[2]);
//             tracedest=atoi(argv[3]);
            if(logtarget == 0)
                return TCL_ERROR;
            return TCL_OK;
        } else if(strcmp(argv[1], "add_trace_target_nam") == 0) {
            logtargetnam[atoi(argv[3])] = (Trace*) TclObject::lookup(argv[2]);
//             tracedest=atoi(argv[3]);
            if(logtargetnam == 0)
                return TCL_ERROR;
            return TCL_OK;
        }
        
    }
    else
    {

        if( (strcmp(argv[1], "install_connection") == 0))
        {
            h_node *node = (h_node *)malloc(sizeof(h_node));
            h_node *list;

            node->route_len = argc - 5;
            for (int tmp=0; tmp < node->route_len; tmp++)
                node->route[tmp] = atoi(argv[5+tmp]);
            node->tag = atoi(argv[2]);

            int hash_val = atoi(argv[2]);
            list = table[hash_val];
            node->next = list;
            table[hash_val] = node;

            return TCL_OK;

        }
    }
    //enter the node number of optical nodes
    if( (strcmp(argv[1], "optic_nodes") == 0))
    {

        for (int tmp=0; tmp < argc - 2; tmp++) {
            opticnodes[atoi(argv[2+tmp])] = 1;
            //printf("atoi(argv[2+tmp]) %d\n",atoi(argv[2+tmp]));
        }

        return TCL_OK;

    }

    //enter the size of FDLs
    if( (strcmp(argv[1], "fdl_size") == 0))
    {

        for (int tmp=0; tmp < argc - 2; tmp++) {
//             printf("size %f num %d\n",atof(argv[2+tmp]), tmp);
            FDLReservation_->fdlsize(atof(argv[2+tmp]), tmp);
            //printf("atoi(argv[2+tmp]) %d\n",atoi(argv[2+tmp]));
        }

        return TCL_OK;

    }
    return (Agent::command(argc,argv));
}

void OpSRAgent::install(int slot, NsObject* p)
{
    if (slot >= nslot_)
        alloc(slot);
    slot_[slot] = p;
    if (slot >= maxslot_)
        maxslot_ = slot;
}








void
OpSRAgent::recv(Packet* packet, Handler*)

{
    if(DEBUG==1) printf("OPSRAgent::recv  at %f\n\n",Scheduler::instance().clock());
    //  int i = 0;
    //int n;
//the burst arrived for with or without routing info


    bool headeraddedfirsttime=0;

    hdr_cmn *cmh =  hdr_cmn::access(packet);
    hdr_src *srh =  hdr_src::access(packet);
    hdr_ip *iph = hdr_ip::access(packet);

    assert(cmh->size() >= 0);
    /* insert the route in the header here , also after this the target
     * should have been set.*/
    if ((!cmh->src_rt_valid)&&(cmh->ptype()!=PT_OP_BURST))
    {
        // source routing info is not available and this packet is not optical packet (it is probably TCP packet)
        if(DEBUG==1) printf("!cmh->src_rt_valid)&(cmh->ptype()!=PT_OP_BURST at %f\n\n",Scheduler::instance().clock());


        h_node *temp_list;
        //cout << " Packet of type: "<< cmh->ptype();

        srh->cur_addr_ = 1;
        int connect_id = iph->daddr();
        //printf("NOW %f (!cmh->src_rt_valid)&(cmh->ptype()!=PT_OP_BURST)) iph->flowid() %d  iph->saddr() %d\n",NOW,iph->flowid(), iph->saddr());
        //cout << " The flowid is" << connect_id << endl;
        temp_list = table[connect_id];

        while (temp_list && temp_list->tag != connect_id)
            temp_list = temp_list->next;
        if (temp_list == NULL)
        {
            printf(" The connection id is not in routing table for normal packet\n");
            exit(0);
        }

        srh->num_addrs_ = temp_list->route_len;
        // cout << " total number of hops:" << srh->num_addrs_ << endl;

        for (int j = 0; j< srh->num_addrs_; j++ )
        {
            srh->addrs[j] = temp_list->route[j];
            // cout << " setting the next hop "<< srh->addrs[j];
        }
        //cout << endl;

        //set the target before starting anything in the tcl code

        cmh->src_rt_valid = '1';
        //      cout << "setting the type to be src_rt" << endl;



        if((opticnodes[srh->addrs[0]]==1)&&(opticnodes[srh->addrs[1]]==1)) {
            //This is an optical node. Destination is an optical node. Send it to optical classifier
            if(DEBUG==1) printf("OPSRAgent::recv will send  route_length %d at %f\n\n", hdr_burst::access(packet)->route_length, Scheduler::instance().clock());
            send(packet,0);

        } else {

            //This node or destination node is not an optical node

            if (srh->cur_addr_ == srh->num_addrs_)
            {
                // supposedly dest reached
                // I can change the packet type and send it to the entry point once again..... this would set things right. This part of the code is managing things at the receiver also..... finally the packet goes to the right agent and not our src rt agent.
                //cout << " setting the packet tye as tcp" << endl;
                cmh->src_rt_valid = '\0';
                send(packet,0);
                return;
            }

            int slot_no = srh->addrs[srh->cur_addr_++];
            NsObject *node =  slot_[slot_no];
            //cout << "passing to the next hop: " << slot_no << endl;
            //printf("OPSRAgent::recv chose node for slot %d\n",slot_no);
            if (node == NULL)
            {
                //cout << "src_rt : null node " << slot_no << " accessed" << endl;
                //cout << "flow id is " << iph->flowid() << endl;
                printf("OPSRAgent::recv node == NULL ERROR srh->addrs[srh->cur_addr_-1] %d srh->cur_addr_-1 %d\n",srh->addrs[srh->cur_addr_-1], srh->cur_addr_-1);
                exit(0);
            }
            else
            {
                if(DEBUG==1) printf("OPSRAgent::recv node != NULL srh->addrs[srh->cur_addr_-1] %d srh->cur_addr_-1 %d\n",srh->addrs[srh->cur_addr_-1], srh->cur_addr_-1);
                cmh->src_rt_valid = '1';
                //printf("OPSRAgent::recv will send to next node %d with srh->addrs[srh->cur_addr_] %d and srh->cur_addr_ %d at %f\n\n", node, srh->addrs[srh->cur_addr_], srh->cur_addr_, Scheduler::instance().clock());
                node->recv(packet, (Handler *)0);
            }
        }
        return;
    }

    if(DEBUG==1) printf("OPSRAgent::recv route_length %d at %f\n\n",hdr_burst::access(packet)->route_length,Scheduler::instance().clock());


    //printf("before !cmh->src_rt_valid at %f\n\n",Scheduler::instance().clock());



    if (!cmh->src_rt_valid)
    {
        //header is not available, add the header. This is an optical control packet. Control packet includes a reference to the optical burst packet.

        //printf("enter !cmh->src_rt_valid at %f\n\n",Scheduler::instance().clock());

        headeraddedfirsttime=1;

        h_node *temp_list;
        //cout << " Packet of type: "<< cmh->ptype();

        srh->cur_addr_ = 1;
        int connect_id = iph->daddr();
        //printf("NOW %f iph->flowid() %d  iph->saddr() %d\n",NOW,iph->flowid(), iph->saddr());
        //cout << " The flowid is" << connect_id << endl;
        temp_list = table[connect_id];

        while (temp_list && temp_list->tag != connect_id)
            temp_list = temp_list->next;
        if (temp_list == NULL)
        {
            printf("%f The connection id %d is not in routing table for burst\n",NOW,  connect_id);
            exit(0);
        }

        srh->num_addrs_ = temp_list->route_len;
        // cout << " total number of hops:" << srh->num_addrs_ << endl;

        for (int j = 0; j< srh->num_addrs_; j++ )
        {
            srh->addrs[j] = temp_list->route[j];
            // cout << " setting the next hop "<< srh->addrs[j];
        }
        //cout << endl;
        //set the target before starting anything in the tcl code
        if(DEBUG==1) printf("OPSRAgent::recv !cmh->src_rt_valid route_length %d at %f\n\n",hdr_burst::access(packet)->route_length,Scheduler::instance().clock());
        cmh->src_rt_valid = '1';
        //      cout << "setting the type to be src_rt" << endl;
        if(DEBUG==1) printf("2OPSRAgent::recv !cmh->src_rt_valid route_length %d at %f\n\n",hdr_burst::access(packet)->route_length,Scheduler::instance().clock());





        //Also add the routing info to the burst inside the control packet
        hdr_burst* burst = hdr_burst::access(packet);
        hdr_cmn *cmhb =  hdr_cmn::access(burst->burst);
        hdr_src *srhb =  hdr_src::access(burst->burst);

        srhb->cur_addr_ = 1;
        //printf("NOW %f iph->flowid() %d  iph->saddr() %d\n",NOW,iph->flowid(), iph->saddr());
        //cout << " The flowid is" << connect_id << endl;

        srhb->num_addrs_ = temp_list->route_len;
        // cout << " total number of hops:" << srh->num_addrs_ << endl;

        for (int j = 0; j< srhb->num_addrs_; j++ )
        {
            srhb->addrs[j] = temp_list->route[j];
            // cout << " setting the next hop "<< srh->addrs[j];
        }
        //cout << endl;
        //set the target before starting anything in the tcl code

        cmhb->src_rt_valid = '1';
        //      cout << "setting the type to be src_rt" << endl;

        //increase current address of burst here
        srhb->cur_addr_++;

    }


    if(DEBUG==1) printf("after !cmh->src_rt_valid route_length %d at %f\n\n",hdr_burst::access(packet)->route_length,Scheduler::instance().clock());

    //printf("OPSRAgent::recv cmh->src_rt_valid at %f\n\n",Scheduler::instance().clock());
    //cout << "src rt packet" << endl;

    if (srh->cur_addr_ == srh->num_addrs_)
    {
        if(DEBUG==1) printf("OPSRAgent::recv REACHED srh->cur_addr_ == srh->num_addrs_ at %f\n\n",Scheduler::instance().clock());
        // supposedly dest reached
        // I can change the packet type and send it to the entry point once again..... this would set things right. This part of the code is managing things at the receiver also..... finally the packet goes to the right agent and not our src rt agent.
        //cout << " setting the packet tye as tcp" << endl;
        cmh->src_rt_valid = '\0';
        send(packet,0);
        return;
    }


    int slot_no = srh->addrs[srh->cur_addr_++];
    NsObject *node =  slot_[slot_no];
    //cout << "passing to the next hop: " << slot_no << endl;


    //printf("OPSRAgent::recv slot_no %d srh->cur_addr_++ %d  slot_[slot_no] %d at %f\n\n",slot_no,srh->cur_addr_,slot_[slot_no],Scheduler::instance().clock());


    if (node == NULL)
    {
        //cout << "src_rt : null node " << slot_no << " accessed" << endl;
        //cout << "flow id is " << iph->flowid() << endl;
        printf("OPSRAgent::recv NULL ERROR 2 with headeraddedfirsttime %d iph->saddr() %d iph->daddr() %d srh->cur_addr_ %d srh->num_addrs_ %d slot_no %d at %f\n\n",headeraddedfirsttime, iph->saddr(), iph->daddr(), srh->cur_addr_, srh->num_addrs_, slot_no, Scheduler::instance().clock());
        exit(0);
    }
    else
    {
        if(DEBUG==1) printf("OPSRAgent::recv  with route_length %d headeraddedfirsttime %d iph->saddr() %d iph->daddr() %d srh->cur_addr_ %d srh->num_addrs_ %d slot_no %d at %f\n\n",hdr_burst::access(packet)->route_length,headeraddedfirsttime, iph->saddr(), iph->daddr(), srh->cur_addr_, srh->num_addrs_, slot_no, Scheduler::instance().clock());

        if (cmh->ptype()!=PT_OP_BURST)
        {
            //electronic domain packet in transit
            //printf("SRagent electronic domain transit\n");
            node->recv(packet, (Handler *)0);

        } else {
// if(DEBUG==1) printf("1OPSRAgent::recv route_length %d type %d slot_no %d srh->cur_addr_++ %d   at %.25f\n\n",hdr_burst::access(packet)->route_length, hdr_burst::access(packet)->burst_type, slot_no, srh->cur_addr_,  Scheduler::instance().clock());
            //Burst in transit

            //cout << "packet of type" << cmh->ptype() << endl;
            cmh->src_rt_valid = '1';
            //send it to the link
// if(DEBUG==1) printf("2OPSRAgent::recv route_length %d type %d slot_no %d srh->cur_addr_++ %d  at %.25f\n\n",hdr_burst::access(packet)->route_length, hdr_burst::access(packet)->burst_type, slot_no, srh->cur_addr_,  Scheduler::instance().clock());
            //printf("OPSRAgent::recv slot_no %d srh->cur_addr_ %d \n\n",slot_no,srh->cur_addr_);
            //node->recv(packet, (Handler *)0);
            hdr_burst* burst = hdr_burst::access(packet);

// if(DEBUG==1) printf("3OPSRAgent::recv route_length %d type %d slot_no %d srh->cur_addr_++ %d   at %.25f\n\n",hdr_burst::access(packet)->route_length, burst->burst_type, slot_no, srh->cur_addr_,  Scheduler::instance().clock());

            if(DEBUG==1) printf("OPSRAgent::recv route_length %d type %d slot_no %d srh->cur_addr_++ %d   at %.25f\n\n",hdr_burst::access(packet)->route_length, burst->burst_type, slot_no, srh->cur_addr_,  Scheduler::instance().clock());

            
            if(ackdontburst==1) {
// 	        //dirty solution.  Acks are not burstified
                if(burst->ack==1) {
                    if(burst->burst_type==0) {
                        node->recv(burst->burst, (Handler *)0);
                        if(logtarget[slot_no]!=NULL){
                            logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                        }
                    }
                    if(burst->burst_type==1) {

                        //if you want to add transmission delay to acks at each hop, remove the comment of following line
                        //burst->first_link=1;

                        node->recv(packet, (Handler *)0);
                    }


                    return;
                }
            }

            if(headeraddedfirsttime==1) {
                hdr_burst* burstdh = hdr_burst::access(burst->burst);
                burst->route_length=srh->num_addrs_-1;
                burst->route_length_tot=srh->num_addrs_-1;
                burstdh->route_length=burst->route_length;
                burstdh->route_length_tot=burst->route_length_tot;
            }

            int result;
            //printf("LinkReservation_[slot_no].slotnumber %d\n",LinkReservation_[slot_no].slotnumber);



            burst->delayedresv=0;	//clear delay information
            
            result=LinkReservation_[slot_no].recv(packet,conversiontype_,0);
            if(DEBUG==1) printf("LinkReservation_ result %d at %.15f\n",result,NOW);

            if(result==1) {
                //Did reservation for control packet

                if(headeraddedfirsttime==1) {
		    //this is the first link of the optical path
                    //data packet is also in the control packet header. Convert them to optical domain and send to the network separately
                    //printf("%f\theaderaddedfirsttime%d\n",NOW,slot_no);
                    hdr_burst* burstdh = hdr_burst::access(burst->burst);
                    burstdh->delayedresv=burst->delayedresv;

                    //printf("%f\theaderaddedfirsttime burst->first_link %d burstdh->first_link %d\n",NOW,burst->first_link, burstdh->first_link);

                    Scheduler& s = Scheduler::instance();
                    
                    double extraswitchtime=0.0;
                    if(JET_TYPE==1){
                        //add switch time in case JET is 1
                        extraswitchtime=SWITCHTIME;
                    }
                    
                    
                    if(DEBUG==1) printf("SRAgent control unique id %d burst unique id %d delay total %.15f reserv %.15f control arrive %.15f burst arrive %.15f in sragent\n",cmh->uid_, hdr_cmn::access(burst->burst)->uid_, HOP_DELAY*burstdh->route_length+burstdh->delayedresv+extraswitchtime, burst->delayedresv, burst->delayedresv+NOW, HOP_DELAY*burstdh->route_length+burstdh->delayedresv+extraswitchtime+NOW);

                    //delay and send control
                    s.schedule(node, packet, burst->delayedresv);

                    //delay and send burst
                    s.schedule(node, burst->burst, HOP_DELAY*burstdh->route_length+burstdh->delayedresv+extraswitchtime);
                    if(DEBUG==1) printf("SRAgent burstdh->route_length  %d \n",burstdh->route_length);
                } else {
		    //this is not the first link
                    //we do not delay the control packets here. They are delayed at the delay agent.
		    //Pass to next node
                    node->recv(packet, (Handler *)0);
                }
            } else if(result==2) {
                //Did reservation for data packet
	        //the amount of FDL delay was written by the scheduler to burst->delayedresv. Delay the data packet by this amount of time
                if((nodetype_!=2)&&(nodetype_!=3)&&(burst->delayedresv>0.00000000000001)) {
                    //we can't say burst->delayedresv==0 because of the problems of double type arithmetic
                    printf("BIG PROBLEM: wrong delay %.25f error in sragent\n",burst->delayedresv);
                    exit(0);
                }
                if(burst->delayedresv<-0.00000000000001) {
                    printf("Negative Delay %.25f unique id %d at %f\n",burst->delayedresv, cmh->uid_,NOW);
                    exit(0);
                }
                if(burst->delayedresv<0.00000000000001) {
                    //we can't say burst->delayedresv==0 because of the problems of double type arithmetic
                    //There is no FDL delay. Pass the burst to next node
		    //we do not delay the control packets here. They are delayed at the delay agent.
                    node->recv(packet, (Handler *)0);
                } else {
                    //There is FDL delay for burst
                    Scheduler& s = Scheduler::instance();

                    //delay in FDL and send the burst
                    s.schedule(node, packet, burst->delayedresv);
		    if(DEBUG==1) printf("LinkReservation_ Did reservation for data packet burst->delayedresv %.15f at %.15f\n",burst->delayedresv,NOW);
                }
            } else if(result==-2) {
                if(DEBUG==1) printf("DROP result==-2 slot_no %d at %f \n",slot_no,NOW);
                
                //No reservation for data burst. Drop it.
                Packet **p = (Packet**) packet->accessdata();
                for(unsigned int i=0; i<burst->packet_num; i++) {
                    if(logtarget[slot_no]!=NULL){
                        logtarget[slot_no]->recv(*p,logtargetnam[slot_no]);
                    }
                    p++;
                }
                if(logtarget[slot_no]!=NULL){
                    logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                }
            } else if(result==-1) {
                //Could not do reservation for control packet. Try FDL or converter if they are available
                if(DEBUG==1) printf("headeraddedfirsttime %d nodetype_ %d at %f\n",headeraddedfirsttime,nodetype_,NOW);
                if(headeraddedfirsttime==1) {
                    //It is unnecessary to try FDLs or converters even if they are available
                    //because the packet is still in electronic domain. We tried electronic buffering but failed, so drop the control and burst packets
                    hdr_burst* burstdh = hdr_burst::access(burst->burst);
                    if(DEBUG==1) printf("SRAgent headeraddedfirsttime 1 burstdh->packet_num  %d \n",burstdh->packet_num);
                    Packet **p = (Packet**) packet->accessdata();
                    for(unsigned int i=0; i<burst->packet_num; i++) {
                        if(logtarget[slot_no]!=NULL){
                            logtarget[slot_no]->recv(*p,logtargetnam[slot_no]);
                        }
                        p++;
                    }
                    if(logtarget[slot_no]!=NULL){
                        logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                    }
                    
                    if(DEBUG==1) printf("DROP result==-1 at %f\n",NOW);
                } else if(nodetype_==0) {
                    //there is no converter nor FDL in this node. Just drop the control packet
                    if(logtarget[slot_no]!=NULL){
                        logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                    }
                } else if(nodetype_==1) {
                    //There are limited number of converters in the node. Check them
                    int k=0;
                    while(k<converternumber_) {
                        result=ConverterReservation_->recv(packet, 0, k);
                        if(result==1) {
                            if(DEBUG==1) printf("converter available %.15f\n",NOW);
                            //converter available. Try scheduling with conversion
                            result=LinkReservation_[slot_no].recv(packet,1,0);
                            if(result==-1) {
                                //No wavelength available. Drop the control packet
                                if(logtarget[slot_no]!=NULL){
                                    logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                                }
                                if(DEBUG==1) printf("wavelength conversion could not rescue the packet %.15f\n",NOW);
                            } else if(result==1) {
                                //reservation is possible with wavelength conversion
                                //do converter reservation
                                result=ConverterReservation_->recv(packet, 1, k);
                                //send the control packet
                                node->recv(packet, (Handler *)0);
                                if(DEBUG==1) printf("wavelength conversion could rescue the packet %.15f\n",NOW);

                            } else {
                                printf("BIG PROBLEM: wavelength conversion error in sragent\n");
                                exit(0);
                            }
                            //Do not try other converters
                            k=converternumber_+3;
                        } else {
                            k=k+1;
                        }
                    }
                    if(k==converternumber_) {
                        //No suitable converter available. Drop the control packet
                        if(logtarget[slot_no]!=NULL){
                            logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                        }

                        if(DEBUG==1) printf("Converter tot %d could not rescue the packet %.15f\n",converternumber_,NOW);
                    }



                } else if(nodetype_==2) {
                    //There are FDLs in the node. Check them
                    int k=0;
                    double delaytime;
                    while(k<fdlnumber_) {
                        delaytime=FDLReservation_->recv(packet, 0, k);
                        if(delaytime>0) {
                            if(DEBUG==1) printf("FDL available %.15f\n",NOW);
                            //FDL available. Try scheduling with extra FDL delay.
                            //conversiontype_ is important here. Maybe there is full converter set
                            result=LinkReservation_[slot_no].recv(packet,conversiontype_, delaytime);
                            if(result==1) {
                                //reservation is possible with FDL
                                //do FDL reservation
                                if(DEBUG==1) printf("FDL k %d could rescue the packet %.15f\n",k,NOW);
                                delaytime=FDLReservation_->recv(packet, 1, k);
                                //delay and send the control packet
                                Scheduler& s = Scheduler::instance();
                                s.schedule(node, packet, delaytime);
                                //Do not try any other FDL
                                k=fdlnumber_+3;
                            } else {
                                k=k+1;
                            }
                        } else {
                            k=k+1;
                        }
                    }

                    if(k==fdlnumber_) {
                        //No suitable FDL available. Drop the control packet
                        if(logtarget[slot_no]!=NULL){
                            logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                        }
                        if(DEBUG==1) printf("FDL tot %d could not rescue the packet %.15f\n",fdlnumber_,NOW);
                    }
                } else if(nodetype_==3) {
                    //There are both converters and FDLs in the node. Check them

                    //There are limited number of converters in the node. Check them
                    int k=0;
                    int converteravailable=2;
                    while(k<converternumber_) {
                        result=ConverterReservation_->recv(packet, 0, k);
                        if(result==1) {
                            converteravailable=1;
                            if(DEBUG==1) printf("converter available %.15f\n",NOW);
                            //converter available. Try scheduling with conversion
                            result=LinkReservation_[slot_no].recv(packet,1,0);
                            if(result==-1) {
                                //No wavelength available. Drop the control packet
                                k=converternumber_;
                                if(DEBUG==1) printf("wavelength conversion could not rescue the packet %.15f\n",NOW);
                            } else if(result==1) {
                                //reservation is possible with wavelength conversion
                                //do converter reservation
                                result=ConverterReservation_->recv(packet, 1, k);
                                //send the control packet
                                node->recv(packet, (Handler *)0);
                                if(DEBUG==1) printf("wavelength conversion could rescue the packet %.15f\n",NOW);
                                k=converternumber_+3;

                            } else {
                                printf("BIG PROBLEM: wavelength conversion error in sragent\n");
                                exit(0);
                            }
                            //Do not try other converters

                        } else {
                            k=k+1;
                        }
                    }
                    if(k==converternumber_) {
                        //No suitable converter available. Try FDLs
                        if(DEBUG==1) printf("Converter tot %d could not rescue the packet %.15f\n",converternumber_,NOW);
                        k=0;
                        double delaytime;
                        int fdlavailable=0;
                        while(k<fdlnumber_) {
                            delaytime=FDLReservation_->recv(packet, 0, k);
                            if(delaytime>0) {
                                if(DEBUG==1) printf("FDL available delaytime %.15e at %.15f\n",delaytime, NOW);
                                //FDL available. Try scheduling with extra FDL delay.
                                //conversiontype_ is important here. Maybe there is full converter set
                                fdlavailable=1;
                                result=LinkReservation_[slot_no].recv(packet,conversiontype_, delaytime);
                                if(result==1) {
                                    //reservation is possible with FDL
                                    //do FDL reservation
                                    if(DEBUG==1) printf("FDL k %d could rescue the packet %.15f\n",k,NOW);
                                    delaytime=FDLReservation_->recv(packet, 1, k);
                                    //delay and send the control packet
                                    Scheduler& s = Scheduler::instance();
                                    s.schedule(node, packet, delaytime);
                                    //Do not try any other FDL
                                    k=fdlnumber_+3;
                                } else {
                                    k=k+1;
                                }
                            } else {
			        if(DEBUG==1) printf("FDL NOT available delaytime %.15e at %.15f\n",delaytime, NOW);
                                k=k+1;
                            }
                        }

                        if(k==fdlnumber_) {

                            if(DEBUG==1) printf("FDL tot %d could not rescue the packet %.15f\n",fdlnumber_,NOW);
                        }

                        if((k==fdlnumber_)&&(converteravailable==1)&&(fdlavailable==1)) {
                            //FDL did not work. Try both converter and FDL if both of them are available
                            if(DEBUG==1) printf("Try both converter and FDL %.15f\n",NOW);

                            k=0;
                            double delaytime;
                            while(k<fdlnumber_) {
                                delaytime=FDLReservation_->recv(packet, 0, k);
                                if(delaytime>0) {
                                    if(DEBUG==1) printf("FDL available %.15f\n",NOW);
                                    //FDL available. Try scheduling with extra FDL delay.
                                    //conversiontype_ is important here. Maybe there is full converter set
                                    result=LinkReservation_[slot_no].recv(packet,converteravailable, delaytime);
                                    if(result==1) {
                                        //reservation is possible with FDL
                                        //do FDL reservation
                                        if(DEBUG==1) printf("FDL k %d could rescue the packet %.15f\n",k,NOW);
                                        delaytime=FDLReservation_->recv(packet, 1, k);
                                        //delay and send the control packet
                                        Scheduler& s = Scheduler::instance();
                                        s.schedule(node, packet, delaytime);
                                        //Do not try any other FDL
                                        k=fdlnumber_+3;
                                    } else {
                                        k=k+1;
                                    }
                                } else {
                                    k=k+1;
                                }
                            }

                            if(k==fdlnumber_) {
                                //No suitable converter+FDL available. Drop the control packet
                                if(logtarget[slot_no]!=NULL){
                                    logtarget[slot_no]->recv(packet,logtargetnam[slot_no]);
                                }
                                if(DEBUG==1) printf("Converter+FDL tot %d could not rescue the packet %.15f\n",fdlnumber_,NOW);
                            }

                        }

                    }

                }

            }

        }

    }
    //printf("did sragent\n");
    return;


}




void OpSRAgent::alloc(int slot)
{
    NsObject** old = slot_;
    int n = nslot_;
    if (old == NULL)
        nslot_ = 32;
    while (nslot_ <= slot)
        nslot_ <<= 1;
    slot_ = new NsObject*[nslot_];
    memset(slot_, 0, nslot_ * sizeof(NsObject*));
    for (int i = 0; i < n; ++i)
        slot_[i] = old[i];
    delete [] old;
}

















