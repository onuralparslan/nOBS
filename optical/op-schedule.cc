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



#include "op-schedule.h"
#include <limits>


//comparison for floating point arithmetic. Returns 0 if two numbers are very close, 1 if a is bigger, 2 if b is bigger
int OpSchedule::compare(double a, double b) {
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

int OpSchedule::compare2(double a, double b) {
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


void OpScheduleHandler::handle(Event*)
{
    op_schedule_.resume();
}






int OpSchedule::recv(Packet* p, int conversion, double fdldelay)
{

    hdr_burst* burstdh = hdr_burst::access(p);
    hdr_cmn* comh = hdr_cmn::access(p);

    Packet* p_;



    if(comh->ptype()==PT_OP_BURST) {
        //Head Dropping
        burstdh->seg_type=0;
        if(burstdh->burst_type==0) {
            //control packet
            //head of the control packet arrived to the node

            if(DEBUG==1) printf("OpSchedule::recv CONTROL PACKET %.15f\n\n\n", Scheduler::instance().clock());

// 		started=clock();

            p_=ScheduleBurst(p, conversion, fdldelay);

// 		ended=clock();
// printf("ended %.15f started %.15f  lasted %.15f CLOCKS_PER_SEC %d time %.15f\n",(double) ended, (double) started, (double) (ended-started),CLOCKS_PER_SEC, (double) (double (ended-started))/CLOCKS_PER_SEC);


            if(p_!=0) {
                return 1;
            } else {
                return -1;
            }

        } else {
            //data burst
            //tail of the burst arrived to the node

            p_=EnterBurst(p);

            if(DEBUG==1) printf("OpSchedule::recv BURST PACKET %.15f\n\n\n", Scheduler::instance().clock());

            if(p_!=0) {
                return 2;
            } else {
                return -2;
            }
        }
    }

    printf("BIG ERROR IN OpSchedule\n");
    exit(0);
}

void OpSchedule::resume()
{
//printf("OpSchedule::resume at %f\n",Scheduler::instance().clock());
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

Packet* OpSchedule::ScheduleBurst(Packet* p_, int conversion, double fdldelay) {
//Applies JET reservation scheme shown in Fig. 1.a and Fig. 1.b in the paper
//C. Qiao and M. Yoo, “Optical Burst Switching (OBS) - A New Paradigm for an Optical Internet”, J. High. Speed Networks, Vol. 8, No. 1, pp. 69-84, Jan. 1999

    double min_gap=std::numeric_limits<double>::max();
    int bestlambda=-1;
    int k;
    int is_head=0;
    int delaystop=0;
    int finishit=0;
    int stop=0;

    double arrival;
    double leave;
    double burst_duration;
    double switch_time=SWITCHTIME;
    double linkspeed=LINKSPEED;
    double realarrival=0;
    double maximum_delay=0;

    hdr_burst* burstch = hdr_burst::access(p_);



    if(DEBUG==1) printf("burstch->route_length*HOP_DELAY %.15f\n\n\n\n",burstch->route_length*HOP_DELAY);
    if(DEBUG==1) printf("(burstch->burst_size*8) %d\n\n\n\n",(burstch->burst_size*8));
    if(DEBUG==1) printf("((int)burstch->burst_size*8)/linkspeed %.15f\n\n\n\n",(double)(((int)burstch->burst_size*8)/(double)linkspeed));
// if(DEBUG==1) printf("min_gap %.15f\n",min_gap);








    if(JET_TYPE==0) {
        if(burstch->first_link==1) {

            if(DEBUG==1) printf("JET_TYPE 0 this is the first link burstch->route_length %d NOW  %.15f with slotnumber %d\n",burstch->route_length,NOW,slotnumber);
            arrival=NOW + burstch->route_length*HOP_DELAY;
            leave  =NOW + burstch->route_length*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed);
            realarrival=arrival;
            //calculate maximum allowed delay reservation time.

            maximum_delay=(double)(((int)((MAX_PACKET_NUM*MAX_MTU)+BURST_HEADER)*8*MAX_DELAYED_BURST)/(double)linkspeed);
            if(DEBUG==1) printf("arrival %.15f leave %.15f maximum_delay %.15f\n",arrival,leave, maximum_delay);


        } else {
            if(DEBUG==1) printf("JET_TYPE 0 this is NOT the first link burstch->route_length %d burstch->route_length_tot %d NOW  %.15f with slotnumber %d\n",burstch->route_length,burstch->route_length_tot,NOW,slotnumber);
            //A strange way of calculation due to precision problems of double type arithmetic

            double hopdelay=burstch->route_length_tot*HOP_DELAY;
            double minushopdelay=0;
            for(k=0; k<burstch->route_length_tot-burstch->route_length - 1; k++) {
                minushopdelay=minushopdelay+HOP_DELAY;
            }


            double transfer=burstch->burst_size*8/linkspeed;
            arrival=NOW + hopdelay -minushopdelay +fdldelay;
            leave  =NOW + hopdelay -minushopdelay + transfer +fdldelay;
            if(DEBUG==1) printf("arrival %.15f leave %.15f hopdelay %.15f minushopdelay %.15f hopdelay -minushopdelay %.15f transfer %.15f fdldelay %.15f\n",arrival,leave, hopdelay , minushopdelay , hopdelay -minushopdelay, transfer, fdldelay);
        }
    } else if(JET_TYPE==1) {
        if(burstch->first_link==1) {

            if(DEBUG==1) printf("JET_TYPE 1 this is the first link burstch->route_length %d NOW  %.15f with slotnumber %d\n",burstch->route_length,NOW,slotnumber);
            arrival=NOW + burstch->route_length*HOP_DELAY + SWITCHTIME;
            leave  =NOW + burstch->route_length*HOP_DELAY + (double)(((int)burstch->burst_size*8)/(double)linkspeed) + SWITCHTIME;
            realarrival=arrival;
            //calculate maximum allowed delay reservation time.

            maximum_delay=(double)(((int)((MAX_PACKET_NUM*MAX_MTU)+BURST_HEADER)*8*MAX_DELAYED_BURST)/(double)linkspeed);
            if(DEBUG==1) printf("arrival %.15f leave %.15f maximum_delay %.15f\n",arrival,leave, maximum_delay);


        } else {
            if(DEBUG==1) printf("JET_TYPE 1 this is NOT the first link burstch->route_length %d burstch->route_length_tot %d NOW  %.15f with slotnumber %d\n",burstch->route_length,burstch->route_length_tot,NOW,slotnumber);
            //A strange way of calculation due to precision problems of double type arithmetic

            double hopdelay=burstch->route_length_tot*HOP_DELAY;
            double minushopdelay=0;
            for(k=0; k<burstch->route_length_tot-burstch->route_length - 1; k++) {
                minushopdelay=minushopdelay+HOP_DELAY;
            }


            double transfer=burstch->burst_size*8/linkspeed;
            arrival=NOW + hopdelay -minushopdelay +fdldelay + SWITCHTIME;
            leave  =NOW + hopdelay -minushopdelay + transfer +fdldelay + SWITCHTIME;
            if(DEBUG==1) printf("arrival %.15f leave %.15f hopdelay %.15f minushopdelay %.15f hopdelay -minushopdelay %.15f transfer %.15f fdldelay %.15f\n",arrival,leave, hopdelay , minushopdelay , hopdelay -minushopdelay, transfer, fdldelay);
        }
    } else {
        printf("OpFDLSchedule Wrong JET_TYPE\n");
        exit(0);
    }






    burst_duration=leave-arrival;
    //Header drop policy

    SchList* temp;
    SchList* besttemp;







    for(k=0; k<MAX_LAMBDA; k++) {
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





    int lambdastart=0;
    int lambdaend=0;
    if((conversion==1)|(burstch->lambda==-1)) {
        //full wavelength conversion is possible in the node or burst is still in electonic domain so we can choose a lambda
        lambdastart=0;
        lambdaend=MAX_LAMBDA;
    } else if(conversion==2) {
        //wavelength conversion not possible or there is a lambda selected before for this burst
        lambdastart=burstch->lambda;
        lambdaend=burstch->lambda+1;
    } else {
        printf("Error: conversion set to %d, which is not valid\n",conversion);
        exit(0);
    }
    if(DEBUG==1) printf("conversion %d burstch->lambda %d lambdastart %d lambdaend %d\n",conversion,burstch->lambda,lambdastart, lambdaend);


    //check the wavelengths
    for(k=lambdastart; k<lambdaend; k++) {
        if(DEBUG==1) printf("llllllll k %d\n",k);

        temp=Head[k];

        //
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
                if(DEBUG==1) printf("1arrival %f min_gap %f\n",arrival,min_gap);
                if(compare(arrival,min_gap)==2) {
                    if(DEBUG==1) printf("entered 1arrival %f min_gap %f\n",arrival,min_gap);
                    min_gap=arrival-switch_time;
                    bestlambda=k;
                    besttemp=temp;
                }

            } else {


                if((compare((temp->end+switch_time),arrival)!=1)&&(compare(arrival-(temp->end+switch_time),min_gap)==2)) {
                    if(DEBUG==1) printf("k%d entered 1arrival %f min_gap %f\n",k,arrival,min_gap);
                    if(temp->Next==NULL) {
                        if(DEBUG==1) printf("k%d emp->Next==NULL \n",k);
                        min_gap=arrival-(temp->end+switch_time);
                        bestlambda=k;
                        besttemp=temp;
                        finishit=1;
                        if(DEBUG==1) printf("Schedule3 flow %d burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->packet_num, burst_duration, NOW);
                    } else if(compare(temp->Next->start-switch_time,leave)!=2) {
                        if(DEBUG==1) printf("k%d temp->Next->start -switch_time>= leave \n",k);
                        min_gap=arrival-(temp->end+switch_time);
                        bestlambda=k;
                        besttemp=temp;
                        finishit=1;
                        if(DEBUG==1) printf("Schedule4 flow %d burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->packet_num, burst_duration, NOW);
                    }


                }



                if(DEBUG==1) printf("temp->end %.15f temp->end+switch_time %.15f arrival %.15f (temp->end+switch_time) - arrival %.15e temp->end+switch_time %.15f min_gap %.15f NOW %.15f\n",temp->end,temp->end+switch_time,arrival,(temp->end+switch_time) - arrival, temp->end+switch_time,min_gap,NOW);
                if(compare((temp->end+switch_time),arrival)!=2) {
                    finishit=1;
                }

            }
            temp=temp->Next;


        }


    }
    if(DEBUG==1) printf("NOW %.25f leave %.25f\n",NOW, leave);

    p_=ScheduleBurst2( p_, min_gap, bestlambda,  k, is_head, delaystop,  arrival,  leave,  burst_duration,  switch_time, linkspeed,  realarrival,  maximum_delay, temp, besttemp);

    return p_;
}

//ScheduleBurst function is cut into two parts for better debugging. This is the second part.
Packet* OpSchedule::ScheduleBurst2(Packet* p_, double min_gap, int bestlambda, int k, int is_head, int delaystop, double arrival, double leave, double burst_duration, double switch_time, double linkspeed, double realarrival, double maximum_delay,SchList* temp, SchList* besttemp) {


//printf("NOW %.25f leave2 %.25f\n",NOW, leave);
// printf("MAX_LAMBDA2 %d \n",MAX_LAMBDA);

    FILE *out;
    SchList* temp2;

    hdr_burst* burstch = hdr_burst::access(p_);


    double min_gap2=std::numeric_limits<double>::max();

    if((burstch->first_link==1)&&(bestlambda==-1)) {
        //search for delayed reservation. try all wavelengths. No wavelength converter is required because burst
        //is still in the electronic domain
        if(DEBUG==1) printf("search for delayed reservation\n");
        for(k=0; k<MAX_LAMBDA; k++) {

            temp=Head[k];

            while((temp!=NULL)&&(is_head!=1)&&(delaystop!=1)) {

                if(DEBUG==1) printf("Schedule search flow %d temp->start %.7f temp->end %.7f realarrival %.7f max %.7f (temp->end+switch_time) -realarrival %.7f min_gap2 %.7f NOW %.7f \n", temp->flow, temp->start, temp->end, realarrival, maximum_delay, (temp->end+switch_time) -realarrival, min_gap2, NOW);


                if((compare((temp->end+switch_time),realarrival)!=2)&&(compare((temp->end+switch_time)-realarrival,min_gap2)==2)) {
                    arrival=temp->end+switch_time;
                    leave=arrival+burst_duration;

                    //can not delay further
                    if(compare(arrival-realarrival,maximum_delay)==1) {
                        delaystop=1;
                        if(DEBUG==1) printf("Schedule NOT flow %d burstch->delayedresv %.15f max %.15f burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, arrival-realarrival, maximum_delay, burstch->packet_num, burst_duration, NOW);
                    } else {
                        //		printf("k%d entered 1arrival %f min_gap %f\n",k,arrival,min_gap);
                        if(temp->Next==NULL) {
                            //
                            min_gap2=(temp->end+switch_time) -realarrival;
                            min_gap=(temp->end+switch_time) -arrival;
                            bestlambda=k;
                            besttemp=temp;
                            delaystop=1;
                            burstch->delayedresv=arrival-realarrival;

                            if(DEBUG==1) printf("Schedule flow %d burstch->delayedresv %.15f max %.15f burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->delayedresv, maximum_delay, burstch->packet_num, burst_duration, NOW);
                        } else if(compare(temp->Next->start-switch_time,leave)!=2) {
                            //			printf("k%d temp->Next->start -switch_time>= leave \n",k);
                            min_gap2=(temp->end+switch_time) -realarrival;
                            min_gap=(temp->end+switch_time) -arrival;
                            bestlambda=k;
                            besttemp=temp;
                            delaystop=1;
                            burstch->delayedresv=arrival-realarrival;

                            if(DEBUG==1) printf("Schedule2 flow %d burstch->delayedresv %.15f max %.15f burstdh->packet_num %d burst_duration %.15f NOW %.15f \n", burstch->flow, burstch->delayedresv, maximum_delay, burstch->packet_num, burst_duration, NOW);
                        }
                    }
                }


                temp=temp->Next;
            }

        }
    }




    if(bestlambda!=-1) {
        if(DEBUG==1) printf("found a lambda %d WITH NO contention with arrival %.15f leave %.15f\n\n",bestlambda,arrival, leave);
        temp=besttemp;
        //store this lambda inside the control packet
        burstch->lambda=bestlambda;

        if(DEBUG==1) {
            if(bestlambda>6) {

                out = fopen("lambda.txt", "a");
                fprintf(out, "lambda %d at %f\n",bestlambda, NOW);
                fclose(out);
            }
        }

        if(is_head==1) {
            if(DEBUG==1) printf("first reservation WILL BE HEAD on this lambda %d\n",bestlambda);
            temp2=new SchList;

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
                if(DEBUG==1) printf("first reservation on this lambda %d with arrival %.15f leave %.15f \n", bestlambda, arrival, leave);

                Tail[bestlambda]->start=arrival;
                Tail[bestlambda]->end=leave;

                Tail[bestlambda]->source=burstch->source;
                Tail[bestlambda]->destination=burstch->destination;
                Tail[bestlambda]->burst_id=burstch->burst_id;
                Tail[bestlambda]->flow=burstch->flow;


            } else {
                if(DEBUG==1) printf("temp->end+switch_time %.15f arrival %.15f arrival - (temp->end+switch_time) %.15f min_gap %.15f \n", temp->end+switch_time , arrival , arrival - (temp->end+switch_time) , min_gap);
                if((compare((temp->end+switch_time),arrival)!=1)&&(compare(arrival-(temp->end+switch_time),min_gap)==0)) {
                    if(temp->Next==NULL) {

                        if(DEBUG==1) printf("last reservation on this lambda %d with arrival %.15f leave %.15f burstch->burst_id %lu\n", bestlambda, arrival, leave,burstch->burst_id);

                        Tail[bestlambda]->Next = new SchList;
                        Tail[bestlambda]=Tail[bestlambda]->Next;
                        Tail[bestlambda]->Next=NULL;

                        Tail[bestlambda]->start=arrival;
                        Tail[bestlambda]->end=leave;



                        Tail[bestlambda]->source=burstch->source;
                        Tail[bestlambda]->destination=burstch->destination;
                        Tail[bestlambda]->burst_id=burstch->burst_id;
                        Tail[bestlambda]->flow=burstch->flow;


                    } else if(compare(temp->Next->start-switch_time,leave)!=2) {
                        if(DEBUG==1) printf("reservation at the middle on this lambda %d with arrival %.15f leave %.15f \n", bestlambda, arrival, leave);

                        temp2=new SchList;
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
                    printf("BIG PROBLEM3 opschedule\n");
                    exit(0);
                }

            }
        }
        return p_;

    } else {

        out = fopen("cont.txt", "a");
        fprintf(out, "cont flow %d from %d id %lu at %f\n",burstch->flow,burstch->source,burstch->burst_id,NOW);
        fclose(out);


        if(DEBUG==1) printf("RESERVATION IMPOSSIBLE. WILL DROP\n\n");
        //drop(p_);
        return 0;

    }



}




Packet* OpSchedule::EnterBurst(Packet* p_) {

//head dropping policy
    int k;
    int found=0;
    int stop=0;
    hdr_burst* burstdh = hdr_burst::access(p_);


    int dest=burstdh->destination;
    int sour=burstdh->source;
    int id=burstdh->burst_id;

    FILE *out;

    SchList* temp;

//printf("OpSchedule::EnterBurst searching for its reservation\n");
//     printf("MAX_LAMBDA %d\n",MAX_LAMBDA);
//Search for its reservation
    for(k=0; k<MAX_LAMBDA; k++) {
        temp=Head[k];
        //printf("OpSchedule::EnterBurst searching k %d\n",k);
        while((temp!=NULL)&&(found==0)) {
//		printf("OpSchedule::EnterBurst searching not null, temp->destination %d, dest %d, temp->source %d, sour %d, temp->burst_id %d, id %d\n",temp->destination, dest, temp->source, sour, temp->burst_id, id);
            if((temp->destination==dest)&&(temp->source==sour)&&(temp->burst_id==id)) {
                //found the reservation. However, burst start and leave times in the reservation may be a bit different from the real burst start and leave times due to precision problems of double arithmetic

                burstdh->lambda=k;
                burstdh->linkspeed=LINKSPEED;
                burstdh->delayedresv=temp->end-NOW;
                //burstdh->delayedresv=NOW-temp->end;

                double transfertime=(double)(((int)hdr_cmn::access(p_)->size()*8)/(double)LINKSPEED);
                if(DEBUG==1) printf("EnterBurst found temp->destination %d temp->source %d temp->burst_id %lu temp->start %.15f NOW %.15f temp->end %.15f burstdh->delayedresv %.15f NOW+transfertime %.15f temp->end-temp->start %.15f transfertime %.15f \n",temp->destination, temp->source, temp->burst_id, temp->start, NOW, temp->end, burstdh->delayedresv, NOW+transfertime, temp->end-temp->start, transfertime);
                //printf("FDL delay %.25f \n",burstdh->delayedresv);
                //exit "for loop"
                k=MAX_LAMBDA;
                found=1;
            } else {
                temp=temp->Next;
            }
        }
    }


//Delete the reservations for packets which left the node
    for(k=0; k<MAX_LAMBDA; k++) {
        while((Head[k]->Next!=NULL)&&(stop==0)) {
//		printf("probably will Delete reservation of k %d from %f\n",k,temp->updated_end);

            if(Head[k]->end+SWITCHTIME<NOW) {
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




//Recovery impossible. Drop the burst
    if(found==0) {
        if(DEBUG==1) printf("OpSchedule::EnterBurst Could not enter at %f\n\n\n\n",NOW);


        /*
        //drop packets inside the burst
        for(int i=0;i<burstdh->packet_num;i++){
        	drop(burstdh->PacketBuffer[i]);
        }
        */

        out = fopen("not-arr-cont.txt", "a");
        fprintf(out, "num %d source %d id %lu at %.15f\n",burstdh->packet_num,burstdh->source,burstdh->burst_id, NOW);
        fclose(out);

        //drop(p_);



        return 0;
    }
    return p_;
}


SchList* OpSchedule::Previous(long index)
{   SchList* temp=Head[1];
    for(long count=0; count<index-1; count++)
    {   temp=temp->Next;
    }
    return temp;
}

void OpSchedule::init(int slot) {

    slotnumber=slot;
}
