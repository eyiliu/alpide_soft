#! /usr/bin/bash

export PATH=/work/datataking/TB1908_CALICE/eudaq/bin:$PATH

# Start Run Control
killall -q xterm

xterm -T "RunControl" -e 'euRun' &
sleep 2 

xterm -T "DataCollector" -e 'euCliCollector -n TriggerIDSyncDataCollector -t mydatacollector -r tcp://192.168.1.160:44000' &
sleep 1

xterm -T "ALPIDE" -e 'euCliProducer -n JadeProducer -t alpide_2 -r tcp://192.168.1.160:44000' &

xterm -T "ALPIDE" -e 'euCliProducer -n JadeProducer -t alpide_3 -r tcp://192.168.1.160:44000' &

xterm -T "ALPIDE" -e 'euCliProducer -n JadeProducer -t alpide_4 -r tcp://192.168.1.160:44000' &

xterm -T "ALPIDE" -e 'euCliProducer -n JadeProducer -t alpide_5 -r tcp://192.168.1.160:44000' &

# Start TLU Producer
xterm -T "TLU" -e 'euCliProducer -n EudetTluProducer -t eudet_tlu -r tcp://192.168.1.160:44000' & 

# OnlineMonitor
# xterm -T "Monitor" -e 'StdEventMonitor -t StdEventMonitor -r tcp://192.168.1.160.100' &
