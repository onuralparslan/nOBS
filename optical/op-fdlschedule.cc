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



#include "op-fdlschedule.h"

//comparison for floating point arithmetic. Returns 0 if two numbers are very close, 1 if a is bigger, 2 if b is bigger
int OpFDLSchedule::compare(double a, double b) {
    int c=0;
    if(max(fabs(a),fabs(b))!=0) {
        double r=fabs((a-b)/max(fabs(a),fabs(b)));
        if(r<1.0e-15) {
            c=0;
        } else if(a>b) {
            c=1;
        } else {
            c=2;
        }
    } else {
        if(a==b) {
            c=0;
        } else if(a>b) {
            c=1;
        } else {
            c=2;
        }
    }
    return c;
}

int OpFDLSchedule::compare2(double a, double b) {
    int c=0;

    if(a==b) {
        c=0;
    } else if(a>b) {
        c=1;
    } else {
        c=2;
    }

    return c;
}


void OpFDLScheduleHandler::handle(Event*)
{
    op_fdlschedule_.resume();
}






double OpFDLSchedule::recv(Packet* p, int action, int fdlnumber)
{

    hdr_burst* burstdh = hdr_burst::access(p);
// hdr_src* srh = hdr_src::access(p);
    hdr_cmn* comh = hdr_cmn::access(p);

    double result;



    if(comh->ptype()==PT_OP_BURST) {

        burstdh->seg_type=0;
        if(burstdh->burst_type==0) {

            if(DEBUG==1) printf("OpFDLSchedule::recv HEAD PACKET %f\n\n\n", Scheduler::instance().clock());

// 		started=clock();

            result=ScheduleBurst(p, action, fdlnumber);

// 		ended=clock();
//
//
// printf("ended %.15f started %.15f  lasted %.15f CLOCKS_PER_SEC %d time %.15f\n",(double) ended, (double) started, (double) (ended-started),CLOCKS_PER_SEC, (double) (double (ended-started))/CLOCKS_PER_SEC);


            return result;

        } else {

            printf("BIG ERROR 2 IN OpFDLSchedule\n");
        }
    }

    printf("BIG ERROR IN OpFDLSchedule\n");
    exit(0);
}

void OpFDLSchedule::resume()
{
//printf("OpFDLSchedule::resume at %f\n",Scheduler::instance().clock());
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

double OpFDLSchedule::ScheduleBurst(Packet* p_, int action, int fdlnumber) {


    double min_gap=1000000;
    int bestlambda=-1;
    int k;
    int is_head=0;
    int delaystop=0;
    int finishit=0;
    int stop=0;
    int singleburst=FDLBURSTCAP;

    double arrival;
    double leave;
    double burst_duration;
    double switch_time=FDLSWITCHTIME;
    double linkspeed=LINKSPEED;
    double realarrival=0;
    double maximum_delay=0;

// FILE *out;


    hdr_burst* burstch = hdr_burst::access(p_);
// hdr_src* srh = hdr_src::access(p_);
// hdr_cmn* comh = hdr_cmn::access(p_);
// hdr_ip *iph = hdr_ip::access(p_);


    if(DEBUG==1) printf("OpFDLSchedule (burstch->route_length + 1) %.15f\n\n\n\n",(burstch->route_length + 1)*HOP_DELAY);
    if(DEBUG==1) printf("OpFDLSchedule(burstch->burst_size*8) %d\n\n\n\n",(burstch->burst_size*8));
    if(DEBUG==1) printf("OpFDLSchedule ((int)burstch->burst_size*8)/linkspeed %.15f\n\n\n\n",(double)(((int)burstch->burst_size*8)/(double)linkspeed));






    if(JET_TYPE==0) {
        arrival=NOW + (burstch->route_length + 1)*HOP_DELAY ;
        //printf("arrival  %.15f\n",arrival);


        if(singleburst==1) {
            //there can be only one burst at a time inside the fdl
            switch_time=0;
            leave  =NOW + fdl_size[fdlnumber] + (burstch->route_length + 1)*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed);
        } else {
            //there can be multiple bursts inside the fdl
            leave  =NOW + (burstch->route_length + 1)*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed);
        }
    } else if(JET_TYPE==1) {
        arrival=NOW + (burstch->route_length + 1)*HOP_DELAY + SWITCHTIME;
        //printf("arrival  %.15f\n",arrival);


        if(singleburst==1) {
            //there can be only one burst at a time inside the fdl
            switch_time=0;
            leave  =NOW + fdl_size[fdlnumber] + (burstch->route_length + 1)*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed) + SWITCHTIME;
        } else {
            //there can be multiple bursts inside the fdl
            leave  =NOW + (burstch->route_length + 1)*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed) + SWITCHTIME;
        }
    } else {
        printf("OpFDLSchedule Wrong JET_TYPE\n");
        exit(0);
    }




    if(DEBUG==1) printf("OpFDLSchedule this CAN NOT be the first link burstch->route_length %d arrival %.15f leave %.15f singleburst %d NOW %.15f\n",burstch->route_length,arrival, leave, singleburst,NOW);

    burst_duration=leave-arrival;
    //Header drop policy


// 	if(burst_duration>fdl_size[fdlnumber]){
// 		//fdl is too short
// 		return 0
// 	}



    FDLSchList* temp;
    FDLSchList* temp2=NULL;
    FDLSchList* besttemp;






    for(k=0; k<fdlnumber; k++) {
        while((Head[k]->Next!=NULL)&&(stop==0)) {
//		printf("probably will Delete reservation of k %d from %f\n",k,temp->updated_end);

            if(Head[k]->end+switch_time<NOW) {
//			printf("Deleted reservation of k %d from %f\n",k,temp->updated_end);
                temp=Head[k];
                Head[k]=temp->Next;
                delete temp;
            } else {
                //stop here, because there is no need to check the rest of the reservations
                stop=1;
            }
        }
        stop=0;
    }





    k=fdlnumber;

    temp=Head[k];
    if(DEBUG==1) printf("FDL check whether k%d WILL BE THE HEAD temp->end %.15e temp->start-switch_time %.15e leave %.15e arrival %.15e min_gap %.15e\n",k, temp->end, temp->start-switch_time, leave, arrival, min_gap);
    if(temp->end!=-1) {
        if((compare(temp->start-switch_time,leave)!=2)&&(compare(arrival,min_gap)==2)) {
            if(DEBUG==1) printf("k%d WILL BE THE HEAD temp->Next->start -switch_time>= leave \n",k);
            min_gap=arrival-switch_time;
            bestlambda=k;
            besttemp=temp;
            is_head=1;
        }
    }


    finishit=0;

    while((temp!=NULL)&&(is_head!=1)&&(finishit!=1)) {
        if(temp->end==-1) {
            //printf("1arrival %f min_gap %f\n",arrival,min_gap);
            if(compare(arrival,min_gap)==2) {
//                 printf("entered 1arrival %f min_gap %f\n",arrival,min_gap);
                min_gap=arrival-switch_time;
                bestlambda=k;
                besttemp=temp;
            }

        } else {


            if((compare((temp->end+switch_time),arrival)!=1)&&(compare(arrival-(temp->end+switch_time),min_gap)==2)) {
                //		printf("k%d entered 1arrival %f min_gap %f\n",k,arrival,min_gap);
                if(temp->Next==NULL) {
                    //			printf("k%d emp->Next==NULL \n",k);
                    min_gap=arrival-(temp->end+switch_time);
                    bestlambda=k;
                    besttemp=temp;
                    finishit=1;
                    if(DEBUG==1) printf("Schedule3 flow %d burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->packet_num, burst_duration, NOW);
                } else if(compare(temp->Next->start-switch_time,leave)!=2) {
                    //			printf("k%d temp->Next->start -switch_time>= leave \n",k);
                    min_gap=arrival-(temp->end+switch_time);
                    bestlambda=k;
                    besttemp=temp;
                    finishit=1;
                    if(DEBUG==1) printf("Schedule4 flow %d burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->packet_num, burst_duration, NOW);
                }


            }
            //printf("r %d temp->end %.15f NOW %f\n",r,temp->end,NOW);
            if(compare((temp->end+switch_time),arrival)!=2) {
                finishit=1;
            }

        }
        temp=temp->Next;


    }

    if(action==0) {
        //Just checking the FDL. Return whether available or not
        if(bestlambda!=-1) {
// 	  printf("fdl succeeded return fdl_size[fdlnumber] %.15e fdlnumber %d \n",fdl_size[fdlnumber], fdlnumber);
            return fdl_size[fdlnumber];
        } else {
// 	  printf("fdl failed, return zero\n");
            return 0;
        }
    } else if(action==1) {
        //Do the reservation


        ScheduleBurst2( p_, min_gap, bestlambda,  k, is_head, delaystop,  arrival,  leave,  burst_duration,  switch_time, linkspeed,  realarrival,  maximum_delay, temp,  temp2,  besttemp);

        return fdl_size[fdlnumber];
    } else {
        printf("BIG PROBLEM: action type error\n");
        exit(0);
    }

}


void OpFDLSchedule::fdlsize(double size, int number) {
    //store the fdl size
    if(number>=fdlnumber) {
        printf("fdlsize Error. Out of bounds. size %f num %d tot %d\n",size, number,fdlnumber);
        exit(0);
    }

    fdl_size[number]=size;

}



void OpFDLSchedule::ScheduleBurst2(Packet* p_, double min_gap, int bestlambda, int k, int is_head, int delaystop, double arrival, double leave, double burst_duration, double switch_time, double linkspeed, double realarrival, double maximum_delay,FDLSchList* temp, FDLSchList* temp2, FDLSchList* besttemp)
{





// FILE *out;


    hdr_burst* burstch = hdr_burst::access(p_);
// hdr_src* srh = hdr_src::access(p_);
// hdr_cmn* comh = hdr_cmn::access(p_);
// hdr_ip *iph = hdr_ip::access(p_);






    if(bestlambda!=-1) {
        if(DEBUG==1) printf("found a fdl %d WITH NO contention with arrival %.15f leave %.15f\n\n",bestlambda,arrival, leave);
        temp=besttemp;

        if(is_head==1) {
            if(DEBUG==1) printf("first reservation WILL BE HEAD on this fdl %d\n",bestlambda);
            temp2=new FDLSchList;

            temp2->start=arrival;
            temp2->end=leave;

            temp2->source=burstch->source;
            temp2->destination=burstch->destination;
            temp2->burst_id=burstch->burst_id;
            temp2->flow=burstch->flow;

            temp2->Next=Head[bestlambda];
            Head[bestlambda]=temp2;
        } else {

            if(temp->end==-1) {
                if(DEBUG==1) printf("first reservation on this fdl %d\n",bestlambda);

                Tail[bestlambda]->start=arrival;
                Tail[bestlambda]->end=leave;

                Tail[bestlambda]->source=burstch->source;
                Tail[bestlambda]->destination=burstch->destination;
                Tail[bestlambda]->burst_id=burstch->burst_id;
                Tail[bestlambda]->flow=burstch->flow;


            } else {

                if((compare((temp->end+switch_time),arrival)!=1)&&(compare(arrival-(temp->end+switch_time),min_gap)==0)) {
                    if(temp->Next==NULL) {

                        if(DEBUG==1) printf("last reservation on this fdl %d with arrival %f leave %f \n", bestlambda, arrival, leave);

                        Tail[bestlambda]->Next = new FDLSchList;
                        Tail[bestlambda]=Tail[bestlambda]->Next;
                        Tail[bestlambda]->Next=NULL;

                        Tail[bestlambda]->start=arrival;
                        Tail[bestlambda]->end=leave;



                        Tail[bestlambda]->source=burstch->source;
                        Tail[bestlambda]->destination=burstch->destination;
                        Tail[bestlambda]->burst_id=burstch->burst_id;
                        Tail[bestlambda]->flow=burstch->flow;


                    } else if(compare(temp->Next->start-switch_time,leave)!=2) {
                        if(DEBUG==1) printf("reservation at the middle on this fdl %d\n",bestlambda);

                        temp2=new FDLSchList;
                        temp2->start=arrival;
                        temp2->end=leave;


                        temp2->source=burstch->source;
                        temp2->destination=burstch->destination;
                        temp2->burst_id=burstch->burst_id;
                        temp2->flow=burstch->flow;

                        temp2->Next=temp->Next;
                        temp->Next=temp2;


                    } else {
                        printf("BIG PROBLEM2\n");
                        exit(0);
                    }

                } else {
                    printf("BIG PROBLEM3\n");
                    exit(0);
                }

            }
        }




    } else {

        printf("BIG PROBLEM: fdl not available\n");
        exit(0);

    }



}




FDLSchList* OpFDLSchedule::Previous(long index)
{   FDLSchList* temp=Head[1];
    for(long count=0; count<index-1; count++)
    {   temp=temp->Next;
    }
    return temp;
}

void OpFDLSchedule::init(int slot) {

    slotnumber=slot;
}
