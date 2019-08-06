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

#### checkout branch *TB1908_CALICE* 
```
cd alpide_soft
git checkout TB1908_CALICE
```

#### build eudaq moudle  for alpide
```
mkdir build
cd build
cmake -Deudaq_DIR=MY_EUDAQ_PATH/cmake ../
```

