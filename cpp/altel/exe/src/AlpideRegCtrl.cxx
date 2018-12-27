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
  ctrl->SendCommand("INIT");
  reader->Open();
  ctrl->WriteByte(0xa0000000, 0);
  reader->Read(1s);
  ctrl->WriteByte(0xb0000000, 0);

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
