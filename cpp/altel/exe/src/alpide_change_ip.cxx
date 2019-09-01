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
  //  std::string ip_now("192.168.22.21");
  //  std::string ip_new("192.168.22.21");
  
  auto ctrl = JadeRegCtrl::Make("AltelRegCtrl", "{\"IP_ADDRESS\": \"192.168.200.21\",\"IP_UDP_PORT\": 4660}");
  uint16_t ip3_new=131;
  uint16_t ip2_new=169;
  uint16_t ip1_new=133;
  uint16_t ip0_new_base=170;
  
  int16_t ip3 = ctrl->ReadByte(0xFFFFFC18);
  int16_t ip2 = ctrl->ReadByte(0xFFFFFC19);
  int16_t ip1 = ctrl->ReadByte(0xFFFFFC1a);
  int16_t ip0 = ctrl->ReadByte(0xFFFFFC1b);
  
  std::cout<< ip3 << " "<< ip2 << " "<<ip1 << " "<<ip0 <<std::endl;

  ctrl->WriteByte(0xFFFFFCFF, 0);
  ctrl->WriteByte(0xFFFFFC18, ip3_new);
  ctrl->WriteByte(0xFFFFFC19, ip2_new);
  ctrl->WriteByte(0xFFFFFC1a, ip1_new);
  ctrl->WriteByte(0xFFFFFC1b, ip0_new_base);
  
  ip3 = ctrl->ReadByte(0xFFFFFC18);
  ip2 = ctrl->ReadByte(0xFFFFFC19);
  ip1 = ctrl->ReadByte(0xFFFFFC1a);
  ip0 = ctrl->ReadByte(0xFFFFFC1b);

  std::cout<< ip3 << " "<< ip2 << " "<<ip1 << " "<<ip0 <<std::endl;
  
  return 0;
}
