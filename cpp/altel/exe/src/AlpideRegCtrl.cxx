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
  ctrl->SendCommand("INIT");
  
  return 0;

}
