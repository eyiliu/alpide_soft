#include "JadeCore.hh"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>

using namespace std::chrono_literals;

int main(int argc, char **argv){
  JadeUtils::PrintTypeIndexMap();
  auto ctrl = JadeRegCtrl::Make("AltelRegCtrl", "{}");
  auto reader = JadeReader::Make("AltelReader", "{}");
  //ctrl->WriteByte(0xf0000000, 0);
  //std::this_thread::sleep_for(5s);

  ctrl->SendCommand("INIT");
  reader->Open();
  ctrl->SendCommand("START");
  reader->Read(1s);
  ctrl->SendCommand("STOP");

  // int n = 0;
  // while(1){
  //   reader->Read(1ms);
  //   n++;
  //   if(n>1){
  //     break;
  //   }
  // }
  std::cout<<">>>>>>>>>>>>finished"<<std::endl;
  
  return 0;

}
