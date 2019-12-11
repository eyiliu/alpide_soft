#include "FirmwarePortal.hh"
#include "getopt.h"

int main(int argc, char **argv){
  const std::string help_usage("\n\
Usage:\n\
-c json_file: path to json file\n\
-i ip_address: eg. 131.169.133.170 for alpide_0 \n\
-h : Print usage information to standard error and stop\n\
");
  
  int c;
  std::string c_opt;
  std::string i_opt;
  while ( (c = getopt(argc, argv, "c:i:h")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'i':
      i_opt = optarg;
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
    std::cerr<<"\ninvalid options: ";
    while (optind < argc)
      std::cerr<<argv[optind++]<<" \n";
    std::cerr<<"\n";
    return 1;
  }

  ////////////////////////
  //test if all opts
  if(c_opt.empty() || i_opt.empty()){
    std::cerr<<"\ninsufficient options.\n";
    std::cerr<<help_usage;
    std::cerr<<"\n\n\n";
    return 1;
  }
  ///////////////////////

  std::string json_file_path = c_opt;
  
  std::string file_context = FirmwarePortal::LoadFileToString(json_file_path);

  FirmwarePortal fw(file_context, i_opt);
  
  uint32_t ip0 = fw.GetFirmwareRegister("IP0");
  uint32_t ip1 = fw.GetFirmwareRegister("IP1");
  uint32_t ip2 = fw.GetFirmwareRegister("IP2");
  uint32_t ip3 = fw.GetFirmwareRegister("IP3");

  fw.SetAlpideRegister("DISABLE_REGIONS", 3);
  uint64_t disabled_regions = fw.GetAlpideRegister("DISABLE_REGIONS");
  
  std::cout<<"\n\ncurrent ip  " <<ip0<<":"<<ip1<<":"<<ip2<<":"<<ip3<<"\n\n"<<std::endl;
  std::cout<< "DISABLE_REGIONS = "<< disabled_regions<<std::endl;
  return 0;
}
