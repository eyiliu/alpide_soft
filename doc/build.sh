#! /usr/bin/env bash

# using LCG_94 (setup gcc, cmake, root and so on)
source /cvmfs/sft.cern.ch/lcg/views/LCG_94/x86_64-centos7-gcc62-opt/setup.sh

WORKSPACE_DIR=`pwd`
# everything are installed on the folder INSTALL
INSTALL_DIR=$WORKSPACE_DIR/INSTALL

git clone https://github.com/eudaq/eudaq
cd eudaq
git checkout v2.4.5
# Alternatives old eudaq  (v2.4.4 + 1 commit)
# The commit after v2.4.4 fix compile issue!
# git checkout fc77bf805509e910091917bfa4072ced3f5b3480

if [ -d "/afs/cern.ch/user/y/yiliu/public/tlu/extern" ] && [ ! -d "$WORKSPACE_DIR/eudaq/user/tlu/extern" ] 
then
    echo "copy tlu files from afs"
    cp -r /afs/cern.ch/user/y/yiliu/public/tlu/extern $WORKSPACE_DIR/eudaq/user/tlu/extern
fi

mkdir build
cd build

# Adjust here for eudaq options
cmake .. -DUSER_TLU_BUILD=ON -DUSER_EUDET_BUILD=ON -DUSER_STCONTROL_BUILD=ON -DUSER_EXAMPLE_BUILD=ON -DEUDAQ_BUILD_STDEVENT_MONITOR=ON -DUSER_ITKSTRIP_BUILD=ON -DEUDAQ_LIBRARY_BUILD_LCIO=ON -DEUDAQ_INSTALL_PREFIX=$INSTALL_DIR
make install -j4
cd ../../

git clone https://github.com/eyiliu/alpide_soft
cd alpide_soft

git checkout FROZEN2104-eudaq245  
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -Deudaq_DIR=$INSTALL_DIR/cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DALPIDE_BUILD_EUDAQ_PRODUCER=ON
make install -j4
cd ../../
