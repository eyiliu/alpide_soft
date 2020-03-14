#!/usr/bin/env bash

# fhlalpide_ctrl: euRun tlu  168
# fhlalpide:      alpide data monitor 167


killall -q xterm

ssh -X testbeam@131.169.133.168 'killall -q euCliProducer; killall -q euRun'
ssh -X testbeam@131.169.133.168 'killall -q euCliProducer; killall -q euCliCollector; killall -q StdEventMonitor; killall -q euLog'
sleep 1

xterm -T "RUN" -e "ssh -X testbeam@131.169.133.168 '/home/testbeam/DEV2001/soft/INSTALL/bin/euRun -a tcp://55000'" &
sleep 1

xterm -T "TLU" -e "ssh -X testbeam@131.169.133.168 '/home/testbeam/DEV2001/soft/INSTALL/bin/euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://131.169.133.168:55000'" &

#xterm -T "ALPIDE 4" -e "ssh -X testbeam@131.169.133.167 '/home/testbeam/DEV2001/soft/INSTALL/bin/euCliProducer -n JadeProducer -t alpide_4 -r tcp://131.169.133.168:55000'" & 
