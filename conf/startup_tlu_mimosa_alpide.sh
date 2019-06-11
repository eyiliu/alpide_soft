#! /usr/bin/bash

export PATH=/home/itkstrip/star/eudaq/bin:$PATH

# Start Run Control
killall -q xterm

xterm -T "Run Control" -e 'euRun' &
sleep 2 

# xterm -T "Log Collector" -e 'euLog -r tcp://192.168.22.100' &
# sleep 1

xterm -T "Data Collector TLU" -e 'euCliCollector -n TriggerIDSyncDataCollector -t one_dc -r tcp://192.168.22.100:44000' &
sleep 1

xterm -T "ALPIDE" -e 'euCliProducer -n JadeProducer -t alpide -r tcp://192.168.22.100:44000' &

# Start TLU Producer
xterm -T "EUDET TLU Producer" -e 'euCliProducer -n EudetTluProducer -t eudet_tlu -r tcp://192.168.22.100:44000' & 

# Start NI Producer locally connect to LV via TCP/IP
xterm -T "NI/Mimosa Producer" -e 'euCliProducer -n NiProducer -t ni_mimosa -r tcp://192.168.22.100:44000' &
sleep 1

# OnlineMonitor
xterm -T "Online Monitor" -e 'StdEventMonitor -t StdEventMonitor -r tcp://192.168.22.100' & 
