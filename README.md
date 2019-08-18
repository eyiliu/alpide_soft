# alpide_soft

## Path of EUDAQ:

Assumming the EUDAQ is installed in folder:
> **MY_EUDAQ_PATH**   

It means the executable file **euRun** is in folder:
> **MY_EUDAQ_PATH**/bin   

The CMAKE config file **eudaqConfig.cmake** is in folder: 
> **MY_EUDAQ_PATH**/cmake  


##  build ALPIDE software against to EUDAQ
#### clone ALPIDE repository
```
git clone https://github.com/eyiliu/alpide_soft.git
```

#### checkout branch *USTC1908* 
```bash 
cd alpide_soft 
git checkout USTC1908 
```

#### build

Note: Please replace MY_EUDAQ_PATH by your real EUDAQ installation path 
```bash 
mkdir build 
cd build 

cmake -Deudaq_DIR=MY_EUDAQ_PATH/cmake -DALPIDE_INSTALL_PREFIX=MY_EUDAQ_PATH ../ 
make install   
```

The generated eudaq module is:    
> **MY_EUDAQ_PATH**/lib/libeudaq_module_jade.so

## conf/ini 
Assumming the DAQ board has ip address 192.168.22.20   
Please ping the ip to comfirm the connection   

Add a section to EUDAQ **ini** file 
```ini
[Producer.alpide_test]
IP_ADDR=192.168.22.20
```

nothing need to be added to EUDAQ **conf** file

## run  
1. Start EUDAQ run control  
1. Start TLU Poroducr  
1. Start the ALPIDE producer   
```bash
euCliProducer -n JadeProducer -t alpide_test
```
