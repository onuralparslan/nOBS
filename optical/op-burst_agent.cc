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



#include "opticaldefaults.h"
#include "op-burst_agent.h"
#include "op-nullagent.h"

class OpticalNullAgentClass : public TclClass {
public:
    OpticalNullAgentClass() : TclClass("ONA") {
    }
    TclObject* create(int, const char*const*) {
        return (new OpticalNullAgent());
    }
} class_opticalnullagent;

OpticalDefaults* OpticalDefaults::instance_;
class OpticalDefaultsClass : public TclClass {
public:
    OpticalDefaultsClass() : TclClass("OpticalDefaults") {}
    TclObject* create(int, const char*const*) {
        return (new OpticalDefaults());
    }
} class_opticaldefaults;

int hdr_burst::offset_;
static class BurstHeaderClass : public PacketHeaderClass {
public:
    BurstHeaderClass() : PacketHeaderClass("PacketHeader/Burst",
                                               sizeof(hdr_burst)) {
        bind_offset(&hdr_burst::offset_);
    }
} class_bursthdr;


static class BurstClass : public TclClass {
public:
    BurstClass() : TclClass("Agent/Burst") {}
    TclObject* create(int, const char*const*) {
        return (new BurstAgent());
    }
} class_burst;


BurstAgent::BurstAgent() : Agent(PT_OP_BURST)


{
    int i,k,j;
    id=1;
    flowshist = new int*[MAX_DEST];
    for (i = 0; i < MAX_DEST; i++) {
        flowshist[i] = new int[MAX_FLOW_QUEUE];
        for (int j = 0; j < MAX_FLOW_QUEUE; j++)
            flowshist[i][j] = 0;
    }
    stat = fopen("stat.txt","wt");
    //number of nodes in the network. Will be used for setting the timers

    //whether acks are burstified or not
    bind("ackdontburst", &ackdontburst);
    //initializing arrays
    Tmr_ = new SendTmr*[MAX_FLOW_QUEUE];
    burst_data = new Packet**[MAX_DEST];
    for (i=0; i<MAX_DEST; i++)
        burst_data[i] = new Packet*[MAX_FLOW_QUEUE];
    burst_cont = new Packet**[MAX_DEST];
    for (i=0; i<MAX_DEST; i++)
        burst_cont[i] = new Packet*[MAX_FLOW_QUEUE];
    burst_buffer = new Packet***[MAX_DEST];
    for (i=0; i<MAX_DEST; i++) {
        burst_buffer[i] = new Packet**[MAX_FLOW_QUEUE];
        for (j=0; j<MAX_FLOW_QUEUE; j++)
            burst_buffer[i][j] = new Packet*[MAX_PACKET_NUM];
    }
    for(j=0; j<MAX_FLOW_QUEUE; j++) {
        Tmr_[j] = new SendTmr[MAX_DEST];
    }
    opticnodes= new bool[MAX_DEST];

    //define timers for all nodes
    for(i=0; i<MAX_DEST; i++) {
        for(j=0; j<MAX_FLOW_QUEUE; j++) {
            Tmr_[j][i].Tmr_Init(this,i,j);
        }
    }

    //printf("sizeof struct %d\n ",sizeof(hdr_burst));


    //Timeout value
    Timeout=TIMEOUT;

    record=0;

    for(i=0; i<MAX_DEST; i++) {
        for(j=0; j<MAX_FLOW_QUEUE; j++) {
            for(k=0; k<MAX_PACKET_NUM; k++) {
                burst_buffer[i][j][k]=NULL;
            }
        }
    }






    for (int tmp=0; tmp < MAX_DEST; tmp++) {
        opticnodes[tmp] = 0;
    }


}

int BurstAgent::command(int argc, const char*const* argv)
{
    if (argc == 4) {
        if (strcmp(argv[1], "burstsend") == 0) {

            //send the burst if there is available
            if(Tmr_[atoi(argv[3])][atoi(argv[2])].status() == TIMER_PENDING) {
                burstsend(atoi(argv[2]),atoi(argv[3]));
            }

            return (TCL_OK);

        }
    }


    if( (strcmp(argv[1], "burst_create") == 0))
    {
        int i,j;
        for(i=0; i<MAX_DEST; i++) {
            for(j=0; j<MAX_FLOW_QUEUE; j++) {
                burst_data[i][j]= allocpkt();
                burst_cont[i][j]= allocpkt();

                hdr_burst* burstdh = hdr_burst::access(burst_data[i][j]);
                hdr_burst* burstch = hdr_burst::access(burst_cont[i][j]);

                burstdh->packet_num=0;

                burstdh->burst_id=id;
                burstch->burst_id=id;

                id++;

                burstdh->burst_type=1;
                burstch->burst_type=0;


                burstch->first_link=1;
                burstdh->first_link=1;

                burstdh->delayedresv=0;
                burstch->delayedresv=0;

                burstdh->drop=0;


                //size, ttl and destination of data burst
                hdr_ip* iph_data = hdr_ip::access(burst_data[i][j]);
// 			hdr_cmn* cmn_data = hdr_cmn::access(burst_data[i][j]);

                iph_data->ttl()=32;
                iph_data->daddr()=i;

                //size, ttl and destination of control packet
                hdr_ip* iph_cont = hdr_ip::access(burst_cont[i][j]);
// 			hdr_cmn* cmn_cont = hdr_cmn::access(burst_cont[i][j]);

                iph_cont->ttl()=32;
                iph_cont->daddr()=i;
            }
        }

        return TCL_OK;

    }



    //enter the node number of optical nodes
    if( (strcmp(argv[1], "optic_nodes") == 0))
    {

        for (int tmp=0; tmp < argc - 2; tmp++) {
            opticnodes[atoi(argv[2+tmp])] = 1;
            //printf("burstagent optic nodes atoi(argv[2+tmp]) %d\n",atoi(argv[2+tmp]));
        }

        return TCL_OK;

    }

    // If the command hasn't been processed by BurstAgent::command,
    // call the command() function for the base class
    return (Agent::command(argc, argv));
}


void BurstAgent::recv(Packet* pkt, Handler*)
{
    if(DEBUG==1) printf("BurstAgent recv at %.15f\n",Scheduler::instance().clock());

    unsigned int i;
    int recv_destination;
    int recv_source;
    int recv_size;
// int failed_num=0;

//FILE *out;

    hdr_ip* iph = hdr_ip::access(pkt);
    hdr_cmn* comh = hdr_cmn::access(pkt);
    hdr_src* srh = hdr_src::access(pkt);



    if(comh->ptype()!=PT_OP_BURST) {

        //Add the incoming packet to the burst

        if(DEBUG==1) printf("BurstAgent recv !=PT_OP_BURST at %.15f\n",Scheduler::instance().clock());

        //find the destination optical node
        int counter=0;
// 	int lastroute=0;
// 	int found=0;
        //flow queue
        int j=0;
        j=(srh->addrs[0]-2)%MAX_FLOW_QUEUE;

        if(DEBUG==1) printf("before dirty ackdontburst %d srh->addrs[0] %d MAX_FLOW_QUEUE %d at %.15f\n",ackdontburst,srh->addrs[0],MAX_FLOW_QUEUE,Scheduler::instance().clock());
// 	//dirty solution. Acks are not burstified
        if(ackdontburst==1) {
            if(comh->ptype()!=PT_ACK) {
                j=(srh->addrs[0]-2)%MAX_FLOW_QUEUE;
                flowshist[srh->addrs[0]-2][j]++;
            }
        } else {
            j=(srh->addrs[0]-2)%MAX_FLOW_QUEUE;
            flowshist[srh->addrs[0]-2][j]++;
        }
//         if(DEBUG==1) printf("after dirty at %.15f\n",Scheduler::instance().clock());
        int d=srh->cur_addr_-1;
        while(srh->num_addrs_>d) {
// 		printf("srh->num_addrs_ %d srh->addrs[d] %d d %d \n",srh->num_addrs_,srh->addrs[d],d);
            if(opticnodes[srh->addrs[d]]==1) {
                recv_destination=srh->addrs[d];
                d=d+1;
            } else {
                d=srh->num_addrs_;
            }

        }
// 		opticnodes[srh->addrs[srh->cur_addr_-1]]==1

// 	printf("recvdest %d\n",recv_destination);
        if(DEBUG==1) printf("BurstAgent j %d and srh->addrs[srh->cur_addr_] %d at %.15f\n",j,srh->addrs[srh->cur_addr_], NOW);
        recv_source=srh->addrs[srh->cur_addr_-1];
        recv_size=comh->size();
//printf("BurstAgent1 j %d and recv_destination %d srh->addrs[srh->cur_addr_] %d at %.15f\n",j,recv_destination,srh->addrs[srh->cur_addr_], NOW);
        hdr_burst* burstdh = hdr_burst::access(burst_data[recv_destination][j]);
        //printf("BurstAgent2 j %d and srh->addrs[srh->cur_addr_] %d at %.15f\n",j,srh->addrs[srh->cur_addr_], NOW);

        hdr_burst* burstch = hdr_burst::access(burst_cont[recv_destination][j]);
        //printf("BurstAgent3 j %d and srh->addrs[srh->cur_addr_] %d at %.15f\n",j,srh->addrs[srh->cur_addr_], NOW);

        hdr_ip* iph_data = hdr_ip::access(burst_data[recv_destination][j]);
        hdr_cmn* cmn_data = hdr_cmn::access(burst_data[recv_destination][j]);

        hdr_ip* iph_cont = hdr_ip::access(burst_cont[recv_destination][j]);
        hdr_cmn* cmn_cont = hdr_cmn::access(burst_cont[recv_destination][j]);


        /*
        if(comh->ptype()==PT_TCP){
        	hdr_tcp *tcph = hdr_tcp::access(pkt);
        	FILE *out;
        	out = fopen("burst-tcp.txt", "a");
        	fprintf(out, "th->seqno() %d\t at %.15f\n", tcph->seqno() ,NOW);
        	fclose(out);


        	out = fopen("common.txt", "a");
        	fprintf(out, "burst recv tcp th->seqno() %d\t at %.15f\n", tcph->seqno() ,NOW);
        	fclose(out);
        }

        if(comh->ptype()==PT_ACK){
        	hdr_tcp *tcph = hdr_tcp::access(pkt);

        	FILE *out;
        	out = fopen("burst-ack.txt", "a");
        	fprintf(out, "th->seqno() %d\t at %.15f\n", tcph->seqno() ,NOW);
        	fclose(out);


        	out = fopen("common.txt", "a");
        	fprintf(out, "burst recv ack th->seqno() %d\t at %.15f\n", tcph->seqno() ,NOW);
        	fclose(out);
        }


        */




        burstdh->destination=recv_destination;
        burstch->destination=recv_destination;

        burstdh->source=recv_source;
        burstch->source=recv_source;

        burstch->flow=j;
        burstdh->flow=j;


        //schedule timer when the first packet of a burst arrives
        if(burstdh->packet_num==0) {
            Tmr_[j][recv_destination].sched(Timeout);
            cmn_data->size()=BURST_HEADER;
            cmn_cont->size()=BURST_HEADER;
        }


        //extend waiting time when a new packet arrives
        /*
        if(burstdh->packet_num>0){
        	Tmr_[recv_destination].resched(Timeout);
        }
        */

        //insert packet to the burst buffer
        burst_buffer[recv_destination][j][burstdh->packet_num]=pkt;


        //increase the inserted packet count of the burst
        burstdh->packet_num=burstdh->packet_num+1;
        burstch->packet_num=burstch->packet_num+1;
        if(DEBUG==1) printf("BurstAgent srh->num_addrs_ %d counter %d\n",srh->num_addrs_,counter);



        iph_data->daddr()=recv_destination;
        iph_cont->daddr()=recv_destination;

        iph_data->saddr()=recv_source;
        iph_cont->saddr()=recv_source;

        if(MAX_FLOW_QUEUE!=1) {
            iph_data->fid_=iph->fid_;
            iph_cont->fid_=iph->fid_;
        }


        //increase size of the burst
        //if it is the first packet of the segment, add segment header size

        cmn_data->size()=cmn_data->size()+recv_size;
        burstdh->burst_size=cmn_data->size();
        burstch->burst_size=cmn_data->size();



        /*
        	burstdh->route_length=counter;
        	burstch->route_length=burstdh->route_length;
        */



        if(cmn_data->size()>=((MAX_PACKET_NUM*MAX_MTU)+BURST_HEADER)) {
            if(DEBUG==1) printf("BurstAgent size %d ttl %d will be sent\n",cmn_data->size(),iph_data->ttl());
            //printf("BurstAgent size %d ttl %d will be sent\n",cmn_data->size(),iph_data->ttl());
            burstsend(recv_destination,j);
        }

// 	//dirty solution. Acks are not burstified
        if(ackdontburst==1) {
            if(comh->ptype()==PT_ACK) {
                if(DEBUG==1) printf("BurstAgent ackdontburst size %d ttl %d will be sent\n",cmn_data->size(),iph_data->ttl());
                //printf("BurstAgent size %d ttl %d will be sent\n",cmn_data->size(),iph_data->ttl());
                burstdh->ack=1;
                burstch->ack=1;

                cmn_data->size()=recv_size;
                burstdh->burst_size=cmn_data->size();
                burstch->burst_size=cmn_data->size();


                burstsend(recv_destination,j);
            }
        }


    } else {
        if(DEBUG==1) printf("BurstAgent recv =PT_OP_BURST at %.15f\n",Scheduler::instance().clock());
        //Burst arrived to the destination. Start deburstification

        hdr_burst* bursth = hdr_burst::access(pkt);

        if((bursth->burst_type==1)&&(bursth->drop==0)) {


            if(DEBUG==1) printf("BurstAgent received data with id %lu at %.15f\n",bursth->burst_id,Scheduler::instance().clock());
            //printf("BurstAgent received data with id %d at %.15f\n",bursth->burst_id,Scheduler::instance().clock());



            Packet **p = (Packet**) pkt->accessdata();
            for(i=0; i<bursth->packet_num; i++) {
                if(DEBUG==1) printf("BurstAgent received send\n");
                //send it to OpClassifier. OpClassifier will send it to source routing agent for sending to next node or send it to PortClassifier which will forward to its agent.

                hdr_src *srh =  hdr_src::access(*p);
                //increase the cur_addr_ counter by 1, because we warped in optical domain
                srh->cur_addr_ =srh->cur_addr_ +1;
                send(*p, 0);
                if(DEBUG==1) printf("BurstAgent received sent\n");
                p++;
            }


            drop(pkt);
        } else {
            if(bursth->burst_type==0) {
                //control packet reached the destination. Drop it.
                if(DEBUG==1) printf("BurstAgent received control with id %lu at %.15f\n",bursth->burst_id,Scheduler::instance().clock());
                //printf("BurstAgent received control with id %d at %.15f\n",bursth->burst_id,Scheduler::instance().clock());
                drop(pkt);
            } else {
                //drop all the packets inside the burst, because no recovery is possible
                Packet **p = (Packet**) pkt->accessdata();
                for(i=0; i<bursth->packet_num; i++) {
                    drop(*p);
                    p++;
                }
                drop(pkt);

            }

        }

    }

}

void SendTmr::expire(Event *e) {
    //printf("BurstAgent::TMR expire at %.15f \n",Scheduler::instance().clock());
    a_->timeoutsend(dest_,queue_);
}


void BurstAgent::timeoutsend(int dest, int queue)
{
    burstsend(dest,queue);

}




//Send the burst
void BurstAgent::burstsend(int recv_destination, int queue) {



    //first cancel the timer
    if(Tmr_[queue][recv_destination].status() == TIMER_PENDING) {
        Tmr_[queue][recv_destination].cancel();
    }

    Packet* p=burst_data[recv_destination][queue];
    Packet* pc=burst_cont[recv_destination][queue];
// if (hdr_cmn::access(pc)->uid()==1955) exit(0);
    hdr_burst* burstdh = hdr_burst::access(p);
    hdr_burst* burstch = hdr_burst::access(pc);

    burstch->lambda=-1;
    burstdh->lambda=-1;



    if(DEBUG==3) {

        if(burstdh->packet_num>record) {

            record=burstdh->packet_num;
            printf("record %u DEBUG %d at %.15f\n",record, DEBUG,NOW);
        }
    }


    //insert the buffered packets into the burst
    p->allocdata(burstdh->packet_num*sizeof(Packet*));
    /* Taking burst size */
    if (!burstdh->ack) {
        fprintf(stat, "%f %d ",NOW,burstdh->packet_num);
        for (int i = 0; i < 10; i++) {
            fprintf(stat, "%d ",flowshist[i][queue]);
            flowshist[i][queue] = 0;
        }
        fprintf(stat, "\n");
    }
    else {
        //fprintf(ackstat, "%d\n",burstdh->packet_num);
    }
    /**/
    memcpy(p->accessdata(),burst_buffer[recv_destination][queue],burstdh->packet_num*sizeof(Packet*));


    //FILE *out;

    burstch->burst=p;

// 	Scheduler& s = Scheduler::instance();

    //send control packet
    send(pc, 0);
    //schedule and send data burst
    //s.schedule(target_, p, HOP_DELAY);

    //allocate new data burst
    burst_data[recv_destination][queue]= allocpkt();


    burstdh = hdr_burst::access(burst_data[recv_destination][queue]);
    hdr_cmn* cmn_data = hdr_cmn::access(burst_data[recv_destination][queue]);
    hdr_ip* iph_data = hdr_ip::access(burst_data[recv_destination][queue]);

    burstdh->burst_id=id;
    burstdh->packet_num=0;
    burstdh->burst_type=1;
    burstdh->first_link=1;
    burstdh->drop=0;
    iph_data->daddr()=recv_destination;


    burstdh->delayedresv=0;




    //allocate control packet
    burst_cont[recv_destination][queue]= allocpkt();

    burstch = hdr_burst::access(burst_cont[recv_destination][queue]);
// 	hdr_cmn* cmn_cont = hdr_cmn::access(burst_cont[recv_destination][queue]);
    hdr_ip* iph_cont = hdr_ip::access(burst_cont[recv_destination][queue]);

    burstch->burst_id=id;
    burstch->burst_type=0;
    burstch->first_link=1;
    burstch->delayedresv=0;
    iph_cont->daddr()=recv_destination;



    if(DEBUG==1) printf("BurstAgent send id %lu size %d ttl %d prepared recv_destination %d queue %d\n",id, cmn_data->size(),iph_data->ttl(),recv_destination, queue);

    id++;


}

BurstAgent::~BurstAgent() {
    // Release allocated memory
    //Delete Tmr_
    printf("Starting destructor\n");
    for (int i = 0; i < MAX_FLOW_QUEUE; i++)
        delete Tmr_[i];
    delete[] Tmr_;
    //Delete burst_data
    for (int i = 0; i < MAX_DEST; i++) {
        for (int j = 0; j < MAX_FLOW_QUEUE; j++)
            delete burst_data[i][j];
        delete[] burst_data[i];
    }
    delete[] burst_data;
    //Delete burst_cont
    for (int i = 0; i < MAX_DEST; i++) {
        for (int j = 0; j < MAX_FLOW_QUEUE; j++)
            delete burst_cont[i][j];
        delete[] burst_cont[i];
    }
    delete[] burst_cont;
    //Delete burst_buffer
    for (int i = 0; i < MAX_DEST; i++) {
        for (int j=0; j<MAX_FLOW_QUEUE; j++) {
            for (int k = 0; k < MAX_PACKET_NUM; k++) {
                delete burst_buffer[i][j][k];
            }
            delete[] burst_buffer[i][j];
        }
        delete[] burst_buffer[i];
    }
    delete[] burst_buffer;
    fclose(stat);
}
