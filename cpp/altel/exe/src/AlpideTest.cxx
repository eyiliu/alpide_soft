#include "JadeCore.hh"
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <ctime>
#include <regex>
#include <map>
#include <utility>
#include <algorithm>


using namespace std::chrono_literals;

int main(int argc, char **argv){
  JadeUtils::PrintTypeIndexMap();  

  const std::string help_usage("\n\
Usage:\n\
-c config_file: configure file\n\
-h : Print usage information to standard error and stop\n\
");
  
  int c;
  std::string c_opt;
  while ( (c = getopt(argc, argv, "c:h")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'h':
      std::cout<<help_usage;
      return 0;
      break;
    default:
      std::cerr<<help_usage;
      return 1;
    }
  }
  if (optind < argc) {
    std::cerr<<"non-option ARGV-elements: ";
    while (optind < argc)
      std::cerr<<argv[optind++]<<" \n";
    std::cerr<<"\n";
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty()){
    std::cerr<<"configure file [-c] is not specified\n";
    std::cerr<<help_usage;
    return 1;
  }
  //////////////////////////////////

  
  
  std::string config_file = c_opt;  
  std::string config_str = JadeUtils::LoadFileToString(config_file);  

  
  JadeOption opt_conf(config_str);
  JadeOption opt_man = opt_conf.GetSubOption("JadeManager");
  std::string man_type = opt_man.GetStringValue("type");
  JadeOption opt_man_para = opt_man.GetSubOption("parameter");
  JadeManagerSP pman = JadeManager::Make(man_type, opt_man_para);
  pman->Init();
  
  std::cout<<"=========start at "<<JadeUtils::GetNowStr()<<"======="<< std::endl;
  pman->StartDataTaking();
  std::cout<<"========="<<std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(20));
  std::cout<<"========="<<std::endl;
  pman->StopDataTaking();
  std::cout<<"=========exit at "<<JadeUtils::GetNowStr()<<"======="<< std::endl;
  return 0;
}
