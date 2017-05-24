# nOBS v2.1

Optical Burst Switching extension for ns-2 simulator. 

You can find the details of the simulator in the following paper. You can also use it for giving reference to the simulator.

G. Gurel, O. Alparslan and E. Karasan, "nOBS: an ns2 based Simulation Tool for Performance Evaluation of TCP Traffic in OBS Networks," Annals of Telecommunications, vol. 62, no. 5-6, pp. 618-637, May/June 2007. 


# RELEASE NOTES

v2.1 (2017-05-22):

-Fixed a bug in reservation delay calculation when JET_TYPE is 1

-Improved the tracers so that the packets dropped in the optical domain are shown in the trace file and nam

-Added the option BURST_SIZE_THRESHOLD. A burst is sent when the total size of packets (in terms of Bytes) inside the burst passes this threshold

-Updated the example script sim20_7bmulti-cont.tcl so that my-duplex-link and my-duplex-link2 functions automatically set LINKSPEED option


v2.0 (2016-08-10):

-Updated the code for ns version 2.35

-Fixed bugs

-added JET_TYPE option

-added a new example simulation script with updated syntax.


# INSTALLATION

Installation requires replacing some default ns-2 files and adding some new source files. For installation:

1) Install and compile a fresh copy of ns version 2.35 

2) Copy the "optical" folder to ns-2.35 folder

3) Replace the files under "routing, tcl, queue, tcp, common, mdart" folders

4) Add the following code to OBJ_CC in Makefile

	optical/op-delay.o\\  
	optical/op-queue.o\\  
	optical/op-burst_agent.o\\  
	optical/op-classifier.o\\  
	optical/op-classifier-hash.o\\  
	optical/op-classifier-sr.o\\  
	optical/op-sragent.o\\  
	optical/op-queue2.o\\  
	optical/op-schedule.o\\  
	optical/op-converterschedule.o\\  
	optical/op-fdlschedule.o\\  

5) Recompile ns

sim20_7bmulti-cont.tcl file is an example simulation script that shows how to configure and use the simulator. 


# CONTACT

You can send bug reports, patches etc. to Onur Alparslan by email to onuralparslan (at) gmail.com . Please replace (at) with @.
