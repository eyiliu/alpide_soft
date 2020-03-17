#include "FirmwarePortal.hh"
#include "getopt.h"
#include "linenoise.h"

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
  std::string w_opt;
  bool w_opt_enable = false;
  bool r_opt = false;
  bool e_opt = false;
  while ( (c = getopt(argc, argv, "c:i:w:reh")) != -1) {
    switch (c) {
    case 'c':
      c_opt = optarg;
      break;
    case 'i':
      i_opt = optarg;
      break;
    case 'w':
      w_opt_enable = true;
      w_opt = optarg;
      break;
    case 'r':
      r_opt = true;
      break;
    case 'e':
      e_opt = true;
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

  bool flag_init = e_opt;
  bool flag_write = w_opt_enable;
  bool flag_read = r_opt;
  uint64_t number_write = 0;
  if( flag_write ){
    number_write = std::stol(w_opt);
  }
  
  std::string json_file_path = c_opt;

  ///////////////////////
  
  std::string file_context = FirmwarePortal::LoadFileToString(json_file_path);

  FirmwarePortal fw(file_context, i_opt);

  if(flag_init){
    fw.SetFirmwareRegister("FIRMWARE_MODE", 0);
    fw.SetFirmwareRegister("ADDR_CHIP_ID", 0x10);
    fw.SendAlpideBroadcast("GRST");
  
    fw.SetAlpideRegister("CMU_DMU_CONF", 0x70);
    fw.SetAlpideRegister("FROMU_CONF_1", 0x10);
    fw.SetAlpideRegister("FROMU_CONF_2", 0x28);
    fw.SetAlpideRegister("VRESETP", 0x75);
    fw.SetAlpideRegister("VRESETD", 0x93);
    fw.SetAlpideRegister("VCASP", 0x56);
    fw.SetAlpideRegister("VCASN", 0x32);
    fw.SetAlpideRegister("VPULSEH", 0xff);
    fw.SetAlpideRegister("VPULSEL", 0x0);
    fw.SetAlpideRegister("VCASN2", 0x39); // TODO: reset value is 0x40
    fw.SetAlpideRegister("VCLIP", 0x0);
    fw.SetAlpideRegister("VTEMP", 0x0);
    fw.SetAlpideRegister("IAUX2", 0x0);
    fw.SetAlpideRegister("IRESET", 0x32);
    fw.SetAlpideRegister("IDB", 0x40);
    fw.SetAlpideRegister("IBIAS", 0x40);
    fw.SetAlpideRegister("ITHR", 0x32); //empty 0x32; 0x12 data, not full.
    fw.SetAlpideRegister("TEST_CTRL", 0x400); // ?
  
    fw.SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw.SetAlpideRegister("PIX_CONF_GLOBAL", 0x0);
    fw.SetAlpideRegister("PIX_CONF_GLOBAL", 0x1);
    fw.SetAlpideRegister("CHIP_MODE", 0x3c);
    fw.SendAlpideBroadcast("RORST");

    //PLL part
    fw.SetAlpideRegister("DTU_CONF", 0x008d);
    fw.SetAlpideRegister("DTU_DAC",  0x0088);
    fw.SetAlpideRegister("DTU_CONF", 0x0085);
    fw.SetAlpideRegister("DTU_CONF", 0x0185);
    fw.SetAlpideRegister("DTU_CONF", 0x0085);

    fw.SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw.SetAlpideRegister("PIX_CONF_GLOBAL", 0x0);
    fw.SetAlpideRegister("TODO_MASK_PULSE", 0xffff);
    fw.SetAlpideRegister("PIX_CONF_GLOBAL", 0x1);
    fw.SetAlpideRegister("FROMU_CONF_1", 0x10);
    fw.SetAlpideRegister("FROMU_CONF_2", 156); //25ns per dig
    fw.SetAlpideRegister("CHIP_MODE", 0x3d);
    fw.SendAlpideBroadcast("RORST");
    fw.SendAlpideBroadcast("PRST");
    fw.SetFirmwareRegister("TRIG_DELAY", 100); //25ns per dig (FrameDuration?)
    fw.SetFirmwareRegister("GAP_INT_TRIG", 20);
  
    fw.SetFirmwareRegister("FIRMWARE_MODE", 0);
  }  

  if(0){
    uint32_t ip0 = fw.GetFirmwareRegister("IP0");
    uint32_t ip1 = fw.GetFirmwareRegister("IP1");
    uint32_t ip2 = fw.GetFirmwareRegister("IP2");
    uint32_t ip3 = fw.GetFirmwareRegister("IP3");

    std::cout<<"\n\ncurrent ip  " <<ip0<<":"<<ip1<<":"<<ip2<<":"<<ip3<<"\n\n"<<std::endl;

  }
  
  if(flag_write){
    fw.SetAlpideRegister("DISABLE_REGIONS", number_write);
    fw.SetFirmwareRegister("TRIG_DELAY", number_write+1);

  }
  if(flag_read){
    uint64_t disabled_regions = fw.GetAlpideRegister("DISABLE_REGIONS");
    uint64_t trig_delay_frameduration = fw.GetFirmwareRegister("TRIG_DELAY");
    std::cout<< "DISABLE_REGIONS = "<< disabled_regions<<std::endl;
    std::cout<< "TRIG_DELAY = "<< trig_delay_frameduration<<std::endl;
}
  return 0;
}
