#Example script for nOBS. Simulates a dumbell topology.
#It is based on the example script of source-routing. Also see ~ns/src_rtg/README.txt

#Creates a T shaped optical backbone topology with three optical ingress nodes where electronic nodes are connected.
#The electronic nodes carrying TCP senders are connected to optical nodes 0 and 5. The electronic nodes carrying the TCP receivers are connected to optical node 4
#Optical backbone architecture and node numbers are
#
#   0-1
#     |
#     2-3-4
#     |
#   5-6


proc my-duplex-link {ns n1 n2 bw delay queue_method queue_length} {
    $ns optical-duplex-link $n1 $n2 $bw $delay $queue_method
    $ns queue-limit $n1 $n2 $queue_length
    $ns queue-limit $n2 $n1 $queue_length
}


proc my-duplex-link2 {ns n1 n2 bw delay queue_method queue_length} {
    $ns optical-simplex-link $n1 $n2 $bw $delay $queue_method
    $ns simplex-link $n2 $n1 $bw $delay DropTail
    $ns queue-limit $n1 $n2 $queue_length
    $ns queue-limit $n2 $n1 $queue_length
}


#Create a simulator object
set ns [new Simulator]

#Variable Simulation settings: max burst size [50,100,200,300,400,500], timeout = [1:1:10] msec, simulation time: 200 sec, buffer = 2*500*1040 bytes, receive window = 500 packets.
set settings [new OpticalDefaults]
$settings set MAX_PACKET_NUM 20;
#Max number of packets in the burst.
$settings set TIMEOUT 7ms;
#burst sending timeout
$settings set MAX_DEST 87;
#number of nodes
$settings set MAX_DELAYED_BURST 5;
#max. number of (full size) bursts delayed in the reservation
$settings set MAX_FLOW_QUEUE 5;
#maximum number of burst queues for flows
$settings set DEBUG 0;
#output information
$settings set JET_TYPE 1;
#type of JET reservation. Set to 0 for JET reservation scheme shown in Fig. 1.a, where HOP_DELAY includes SWITCHTIME,
#and 1 for JET reservation scheme shown in Fig. 1.b, where HOP_DELAY does not include SWITCHTIME so lower HOP_DELAY can be selected, as in the paper
#C. Qiao and M. Yoo, “Optical Burst Switching (OBS) - A New Paradigm for an Optical Internet”, J. High. Speed Networks, Vol. 8, No. 1, pp. 69-84, Jan. 1999

# The following are the default values for settings, only the above have been changed.
#$settings set MAX_PACKET_NUM 500; #Max number of packets in the burst.
#$settings set HOP_DELAY 0.00001; #node processing time
#$settings set TIMEOUT 0.005; #burst sending timeout
#$settings set MAX_LAMBDA 1; #wavelegth per link
#$settings set LINKSPEED 1Gb; #wavelength speed
#$settings set SWITCHTIME 0.000005; #minumum time difference between two bursts for switching
#$settings set DEBUG 3; #output information
#$settings set MAX_DEST 40; #number of nodes
#$settings set BURST_HEADER 40; #burst header size
#$settings set MAX_DELAYED_BURST 2; max. number of (full size) bursts delayed in the reservation
#$settings set MAX_FLOW_QUEUE 1; #maximum number of burst queues for flows
#$settings set FDLSWITCHTIME 0.000005; #minumum time difference between two bursts for FDL switching. Normally equal to SWITCHTIME
#$settings set FDLBURSTCAP 1; #FDL type. If set to 1, there can be only one burst at a time inside the fdl. Otherwise there can be multiple bursts inside the fdl
#$settings set MAX_MTU 1040; #Maximum size (MTU) of packet entering the burstifier
#$settings set JET_TYPE 0; #type of JET reservation. Set to 0 for JET reservation scheme shown in Fig. 1.a, where HOP_DELAY includes SWITCHTIME,
#and 1 for JET reservation scheme shown in Fig. 1.b, where HOP_DELAY does not include SWITCHTIME so lower HOP_DELAY can be selected, as in the paper
#C. Qiao and M. Yoo, “Optical Burst Switching (OBS) - A New Paradigm for an Optical Internet”, J. High. Speed Networks, Vol. 8, No. 1, pp. 69-84, Jan. 1999



$ns color 12 Red
$ns color 13 Yellow
$ns color 14 Green
$ns color 15 Purple
$ns color 16 Black
$ns color 17 Magenta
$ns color 18 Brown
$ns color 19 Orange
$ns color 20 Red
$ns color 21 Blue

#Open the win size file
set winfile [open windows.txt w]
set goodfile [open goodput.txt w]


# enable source routing
$ns op_src_rting 1


#Open the logfile
set nf [open out.tr w]
$ns trace-all $nf
#Open the nam trace file
set f [open out.nam w]
$ns namtrace-all $f

#Start from zero when numbering the nodes.

#Create 7 optical nodes
for {set i 0} {$i < 7} {incr i} {
#create optical nodes by OpNode
# 	    puts "create optical node $i"

set n($i) [$ns OpNode]
#             	    exit 0
#define optical nodes
    set temp [$n($i) set src_agent_]
    $temp optic_nodes 0 1 2 3 4 5 6;
#id of optical nodes
    $temp set nodetype_ 3;
#If set to 0, there is no converter nor FDL in this node.
#If set to 1, there are limited number of converters in the node
#If set to 2, there are FDLs in the node
#if set to 3, there are converters and FDLs in the node
    $temp set conversiontype_ 1;
#If set to 1, full wavelength conversion is possible in the node or burst is still in electonic domain so we can choose a lambda
#If set to 2, wavelength conversion not possible or there is a lambda selected before for this burst
    $temp set converternumber_ 2
    $temp set fdlnumber_ 3
    $temp create;
#create the necessary arrays in src_agent_ object
    $temp fdl_size 0.001 0.010 0.020

    $temp set ackdontburst 1;
#ACKs are not burstified if set to 1

    set temp [$n($i) set burst_agent_]
    $temp burst_create;
#create the burst arrays
    $temp optic_nodes 0 1 2 3 4 5 6;
#id of optical nodes

    $temp set ackdontburst 1;
#ACKs are not burstified if set to 1

    set temp [$n($i) set classifier_]
    $temp optic_nodes 0 1 2 3 4 5 6;
#id of optical nodes

}



#Create 80 electronic nodes
for {set i 7} {$i < 87} {incr i} {
set n($i) [$ns node]

#define optical nodes
    set temp [$n($i) set src_agent_]
    $temp optic_nodes 0 1 2 3 4 5 6;
#id of optical nodes



    set temp [$n($i) set classifier_]
    $temp optic_nodes 0 1 2 3 4 5 6;
#id of optical nodes

}

set queue_length 100000

#Create links between the nodes
my-duplex-link2 $ns $n(0) $n(1) 1000Mb 10ms OpQueue $queue_length
my-duplex-link2 $ns $n(1) $n(2) 1000Mb 10ms OpQueue $queue_length
my-duplex-link2 $ns $n(2) $n(3) 1000Mb 10ms OpQueue $queue_length
my-duplex-link2 $ns $n(3) $n(4) 1000Mb 10ms OpQueue $queue_length
my-duplex-link2 $ns $n(5) $n(6) 1000Mb 10ms OpQueue $queue_length
my-duplex-link2 $ns $n(6) $n(2) 1000Mb 10ms OpQueue $queue_length

#creating the loss model for dropping packets randomly
set loss_module [new ErrorModel]
$loss_module set rate_ 0.0
$loss_module unit pkt
$loss_module ranvar [new RandomVariable/Uniform]
$loss_module drop-target [new ONA]

#set whether burst or control packet will be dropped. Set to 0 for dropping only burst data packets, 1 for dropping only control packets, 2 for dropping both control and burst packets
$loss_module set opticaldrop_ 2

#Inserting Error Module
$ns lossmodel $loss_module $n(0) $n(1)
for {set i 7} {$i < 27} {incr i} {
$ns duplex-link $n($i) $n(0) 155Mb 1ms DropTail
    $ns queue-limit $n($i) $n(0) $queue_length
    $ns queue-limit $n(0) $n($i) $queue_length
}

for {set i 27} {$i < 47} {incr i} {
$ns duplex-link $n($i) $n(5) 155Mb 1ms DropTail
    $ns queue-limit $n($i) $n(5) $queue_length
    $ns queue-limit $n(5) $n($i) $queue_length
}


for {set i 47} {$i < 87} {incr i} {
$ns duplex-link $n($i) $n(4) 155Mb 1ms DropTail
    $ns queue-limit $n($i) $n(4) $queue_length
    $ns queue-limit $n(4) $n($i) $queue_length
}




set flow 0

for {set i 7} {$i < 47} {incr i} {

set d [expr $i + 40]

#Create a TCP agent and attach it to node n0
    set cbr($i) [new Agent/TCP/Reno]
    $ns attach-agent $n($i) $cbr($i)
    $cbr($i) set fid_ $d
    $cbr($i) set fid2_ $flow
    $cbr($i) set window_ 10000

    $cbr($i) target [$n($i) set src_agent_]

    set ftp($i) [$cbr($i) attach-source FTP]


    set null($i) [new Agent/TCPSink]
    $ns attach-agent $n($d) $null($i)
#$null($i) set fid_ $s  #This line is not working. Hard coded in tcp sink.cc
    $null($i) set fid2_ $flow

    $null($i) target [$n($d) set src_agent_]

    $ns connect $cbr($i) $null($i)

    incr flow
}


for {set i 7} {$i < 27} {incr i} {
set d [expr $i + 40]
    set temp [$n($i) set src_agent_]
    $temp install_connection $d         $i $d   $i 0 4 $d;
#The explanation from source-routing module for this command is below:
#The first argurment here is the flow id for the connection. This is
#same as the flow id of the agent generating the traffic ( TCP or
#UDP). The second arguement is the source node id of the
#connection. The third arguement is the destination node id of the
#connection. The arguements following this form the source route of
#the connection.
    set temp [$n($d) set src_agent_]
    $temp install_connection $i         $d $i   $d 4 0 $i

    $ns at [expr ($i-7.0)/20.0] "$ftp($i) start"
}


for {set i 27} {$i < 47} {incr i} {
set d [expr $i + 40]
    set temp [$n($i) set src_agent_]
    $temp install_connection $d         $i $d   $i 5 4 $d;
#The explanation from source-routing module for this command is below:
#The first argurment here is the flow id for the connection. This is
#same as the flow id of the agent generating the traffic ( TCP or
#UDP). The second arguement is the source node id of the
#connection. The third arguement is the destination node id of the
#connection. The arguements following this form the source route of
#the connection.
    set temp [$n($d) set src_agent_]
    $temp install_connection $i         $d $i   $d 4 5 $i

    $ns at [expr ($i-27.0)/20.0] "$ftp($i) start"
}

set temp [$n(0) set src_agent_]
$temp install_connection 4         0 4   0 1 2 3 4
set temp [$n(4) set src_agent_]
$temp install_connection 0         4 0   4 3 2 1 0

set temp [$n(5) set src_agent_]
$temp install_connection 4         5 4   5 6 2 3 4
set temp [$n(4) set src_agent_]
$temp install_connection 5         4 5   4 3 2 6 5



proc plotWindow {file} {
    global goodfile
    global ns
    global cbr
    set time 0.01
    set now [$ns now]
    puts -nonewline $file "$now"
    puts -nonewline $goodfile "$now"
    for {set i 7} {$i < 17} {incr i} {
    set cwnd($i) [$cbr($i) set cwnd_]
        puts -nonewline $file " $cwnd($i)"
        puts -nonewline $goodfile " "
        puts -nonewline  $goodfile [$cbr($i) set ack_]
#puts -nonewline  $goodfile [expr  [$cbr($i) set ack_]/[expr $now-$i]]
    }
    puts $file ""
    puts $goodfile ""
    $ns at [expr $now+$time] "plotWindow $file"
}

proc finish {} {
    global ns nf
    global nf
    global f
    global winfile
    global goodfile
    $ns flush-trace
#Close the trace file
    close $f
    close $winfile
    close $goodfile
#Execute nam on the trace file
    exec ./nam out.nam
    exit 0
}


$ns at 1 "plotWindow $winfile"
$ns at 10 "finish"
$ns run
