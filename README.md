# alpide_soft

## EUDAQ installation path:  
MY_EUDAQ_PATH 
It means the executable file **euRun** is in folder   **MY_EUDAQ_PATH/bin**   
The CMAKE config file **eudaqConfig.cmake** is in folder   **MY_EUDAQ_PATH/cmake**  


##  build eudaq module for ALPIDE telescope plane
#### clone  repository
```
git clone https://github.com/eyiliu/alpide_soft.git
```

#### checkout branch *USTC1908* 
```
cd alpide_soft
git checkout TB1908_CALICE
```

#### build eudaq moudle  for alpide
```
mkdir build
cd build

cmake -Deudaq_DIR=MY_EUDAQ_PATH/cmake -DALPIDE_INSTALL_PREFIX=MY_EUDAQ_PATH ../
make install  
```
the generated eudaq module is   
**MY_EUDAQ_PATH/libeudaq\_module_jade.so**



### conf/ini 
assumming the DAQ board has ip address 192.168.22.20   
please ping the ip to comfirm the connection   

add a section to EUDAQ ini file 
```
[Producer.alpide_test]
IP_ADDR=192.168.22.20
```

nothing need to be added to EUDAQ conf file

## run  

start the ALPIDE producer
```
euCliProducer -n JadeProducer -t alpide_test
```
